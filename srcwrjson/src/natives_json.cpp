// SPDX-License-Identifier: GPL-3.0-or-later
// Copyright 2022-2025 rtldg <rtldg@protonmail.com>
// This file is part of srcwrtimer (https://github.com/srcwr/srcwrtimer/)

#include "../../extshared/src/extension.h"
#include "../../extshared/src/coreident.hpp"
//#include "../../extshared/src/sprintf.h"
#include <logic/sprintf.h>
#include "rust_exports_json.h"
#include "ISRCWRJSONHello.hpp"

#include <unordered_map>
#include <vector>

template<class T> const T& Zmin(const T& a, const T& b) { return (b < a) ? b : a; }


std::unordered_map<std::string, std::vector<Handle_t>> HandleGroups;

HandleType_t g_JSONType = 0;
extern const sp_nativeinfo_t JSONNatives[];


///////////////////////////////////////////////////////////////////////////////////////

class SRCWRJSONHello : public ISRCWRJSONHello
{
public:
	unsigned int GetInterfaceVersion()
	{
		return SMINTERFACE_JSONHELLO_VERSION;
	}
	const char* GetInterfaceName()
	{
		return SMINTERFACE_JSONHELLO_NAME;
	}
public:
	Handle_t MakeJSONObject(IPluginContext* ctx, const char* s, int len)
	{
		COMPAT_CHECK();
		return g_MyExtension.HandleOrDestroy(ctx, rust_SRCWRJSON_FromString(0, s, len), g_JSONType);
	}
	void* GetUnsafePointer(IPluginContext* ctx, cell_t handy)
	{
		void* object;
		GET_HANDLE(handy, object, g_JSONType);
		return rust_SRCWRJSON_UnsafePointer(object);
	}
};
SRCWRJSONHello g_SRCWRJSONHello;


void MyExtension::OnHandleDestroy(HandleType_t type, void* object)
{
	if (type == g_JSONType)
		rust_handle_destroy_SRCWRJSON(object);
}
bool MyExtension::GetHandleApproxSize(HandleType_t type, void* object, unsigned int* size)
{
	if (type == g_JSONType)
		return rust_handle_size_SRCWRJSON(object, size);
	return false;
}


///////////////////////////////////////////////////////////////////////////////////////

bool Extension_OnLoad(char* error, size_t maxlength)
{
	g_MyExtension.set_compat_version(1);

	if (!sharesys->AddInterface(myself, &g_SRCWRJSONHello))
	{
		smutils->Format(error, maxlength, "Failed to add SRCWRJSONHello interface (to be used by SRCWRHTTP)");
		return false;
	}

	sharesys->AddNatives(myself, JSONNatives);

	g_JSONType = g_pHandleSys->CreateType(
		  "SRCWRJSON"
		, &g_MyExtension
		, 0
		, NULL
		, NULL
		, myself->GetIdentity()
		, NULL
	);

	return true;
}

void Extension_OnUnload()
{
	g_pHandleSys->RemoveType(g_JSONType, myself->GetIdentity());
}

void Extension_OnAllLoaded() {}

///////////////////////////////////////////////////////////////////////////////////////

static cell_t N_SRCWRJSON_SRCWRJSON(IPluginContext* ctx, const cell_t* params)
{
	COMPAT_CHECK();
	bool array = params[1] != 0;
	return g_MyExtension.HandleOrDestroy(ctx, rust_SRCWRJSON_SRCWRJSON(array), g_JSONType);
}

static cell_t N_SRCWRJSON_Clone(IPluginContext* ctx, const cell_t* params)
{
	COMPAT_CHECK();
	void* object;
	GET_HANDLE(params[1], object, g_JSONType);
	return g_MyExtension.HandleOrDestroy(ctx, rust_SRCWRJSON_Clone(object), g_JSONType);
}

static cell_t N_SRCWRJSON_GetHandles(IPluginContext* ctx, const cell_t* params)
{
	COMPAT_CHECK();
	char* name;
	ctx->LocalToString(params[1], &name);
	Handle_t* handles;
	ctx->LocalToPhysAddr(params[2], (cell_t**)&handles); // TODO: ERROR CHECK
	int count = params[3];
	bool owned_by_extension = params[4] != 0;

	if (name[0] == '\0')
	{
		ctx->ReportError("Empty string passed for name");
		return 0;
	}

	// why would anyone use this much???
	#define MAX_GETHANDLES_COUNT 501

	if (count > MAX_GETHANDLES_COUNT || count < 1)
	{
		ctx->ReportError("Bad handle count (%d) passed! Must be `1 <= count <= %d`", count, MAX_GETHANDLES_COUNT);
		return 0;
	}

	std::string groupname = name;

	if (auto search = HandleGroups.find(groupname); search != HandleGroups.end())
	{
		const auto& group = search->second;
		memcpy(handles, group.data(), Zmin((unsigned)count, group.size()) * sizeof(Handle_t));
		return 1;
	}

	void* objects[MAX_GETHANDLES_COUNT] = {};
	rust_SRCWRJSON_GetObjects(objects, count);

	auto owner = owned_by_extension ? NULL : ctx->GetIdentity();

	for (int i = 0; i < count; i++)
	{
		handles[i] = handlesys->CreateHandle(
			  g_JSONType
			, objects[i]
			, owner
			, myself->GetIdentity()
			, NULL
		);

		if (!handles[i])
		{
			// Destroy objects that received handles...
			for (int pre = 0; pre < i; pre++)
			{
				handlesys->FreeHandle(handles[pre], NULL);
				handles[pre] = 0; // sourcepawn array of handles we wrote to...
			}

			// Finish off the rest of the objects that haven't received handles...
			for (int post = i; post < count; post++)
			{
				rust_handle_destroy_SRCWRJSON(objects[post]);
			}

			return 0;
		}
	}

	// lol
	HandleGroups.emplace(
		std::piecewise_construct,
		std::forward_as_tuple(groupname),
		std::forward_as_tuple(handles, handles+count));
	return 1;
}

static cell_t N_SRCWRJSON_ToFile(IPluginContext* ctx, const cell_t* params)
{
	void* object;
	GET_HANDLE(params[1], object, g_JSONType);
	char* filename;
	(void)ctx->LocalToString(params[2], &filename);
	int flags = params[3];
	char filenamebuf[PLATFORM_MAX_PATH];
	smutils->BuildPath(Path_Game, filenamebuf, sizeof(filenamebuf), "%s", filename); // TODO: Error check?
	char* key;
	(void)ctx->LocalToStringNULL(params[4], &key);
	MAYBE_FORMAT(4, key);
	return rust_SRCWRJSON_ToFile(object, filenamebuf, flags, key, fmtlen);
}

static cell_t N_SRCWRJSON_ToFileHandle(IPluginContext* ctx, const cell_t* params)
{
	void* object;
	GET_HANDLE(params[1], object, g_JSONType);
	cell_t filehandle = params[2];
	int flags = params[3];

	HandleError err;
	HandleSecurity sec(g_pCoreIdent, g_pCoreIdent);
	IFileObject* fileobject;

	if ((err = handlesys->ReadHandle(filehandle, g_FileType, &sec, (void**)&fileobject))
		!= HandleError_None)
	{
		ctx->ReportError("Invalid file handle %x (error: %d)", filehandle, err);
		return 0;
	}

	char* key;
	(void)ctx->LocalToStringNULL(params[4], &key);
	MAYBE_FORMAT(4, key);

	return rust_SRCWRJSON_ToFileHandle(object, fileobject, flags, key, fmtlen);
}

static cell_t N_SRCWRJSON_ToString(IPluginContext* ctx, const cell_t* params)
{
	void* object;
	GET_HANDLE(params[1], object, g_JSONType);
	char* buffer;
	(void)ctx->LocalToString(params[2], &buffer);
	cell_t buffer_local_addr = params[2]; // [sic]
	int maxlength = params[3];
	int flags = params[4];
	char* key;
	(void)ctx->LocalToStringNULL(params[5], &key);
	MAYBE_FORMAT(5, key);
	return rust_SRCWRJSON_ToString(ctx, object, buffer, buffer_local_addr, maxlength, flags, key, fmtlen);
}

static cell_t N_SRCWRJSON_FromFile(IPluginContext* ctx, const cell_t* params)
{
	COMPAT_CHECK();
	char* filename;
	(void)ctx->LocalToString(params[1], &filename);
	int flags = params[2];
	char filenamebuf[PLATFORM_MAX_PATH];
	smutils->BuildPath(Path_Game, filenamebuf, sizeof(filenamebuf), "%s", filename); // TODO: Error check?
	return g_MyExtension.HandleOrDestroy(ctx, rust_SRCWRJSON_FromFile(filenamebuf, flags), g_JSONType);
}

static cell_t N_SRCWRJSON_FromFileHandle(IPluginContext* ctx, const cell_t* params)
{
	COMPAT_CHECK();
	cell_t filehandle = params[1];
	int flags = params[2];

	HandleError err;
	HandleSecurity sec(g_pCoreIdent, g_pCoreIdent);
	IFileObject* fileobject;

	if ((err = handlesys->ReadHandle(filehandle, g_FileType, &sec, (void**)&fileobject))
		!= HandleError_None)
	{
		ctx->ReportError("Invalid file handle %x (error: %d)", filehandle, err);
		return 0;
	}

	return g_MyExtension.HandleOrDestroy(ctx, rust_SRCWRJSON_FromFileHandle(fileobject, flags), g_JSONType);
}

static cell_t N_SRCWRJSON_FromString1(IPluginContext* ctx, const cell_t* params)
{
	COMPAT_CHECK();
	char* s;
	(void)ctx->LocalToString(params[1], &s);
	int end = params[2];
	int flags = params[3];
	return g_MyExtension.HandleOrDestroy(ctx, rust_SRCWRJSON_FromString(flags, s, end), g_JSONType);
}

static cell_t N_SRCWRJSON_FromString2(IPluginContext* ctx, const cell_t* params)
{
	COMPAT_CHECK();
	int flags = params[1];
	char* fmt;
	(void)ctx->LocalToString(params[2], &fmt);
	MAYBE_FORMAT(2, fmt);
	return g_MyExtension.HandleOrDestroy(ctx, rust_SRCWRJSON_FromString(flags, fmt, fmtlen), g_JSONType);
}

static cell_t N_SRCWRJSON_Has(IPluginContext* ctx, const cell_t* params)
{
	void* object;
	GET_HANDLE(params[1], object, g_JSONType);
	int flags = params[2];
	char* key;
	(void)ctx->LocalToString(params[3], &key);
	MAYBE_FORMAT(3, key);
	return rust_SRCWRJSON_Has(object, flags, key, fmtlen);
}

static cell_t N_SRCWRJSON_GetType(IPluginContext* ctx, const cell_t* params)
{
	void* object;
	GET_HANDLE(params[1], object, g_JSONType);
	int flags = params[2];
	char* key;
	(void)ctx->LocalToStringNULL(params[3], &key);
	MAYBE_FORMAT(3, key);
	return rust_SRCWRJSON_GetType(object, flags, key, fmtlen);
}

static cell_t N_SRCWRJSON_Type_get(IPluginContext* ctx, const cell_t* params)
{
	void* object;
	GET_HANDLE(params[1], object, g_JSONType);
	return rust_SRCWRJSON_GetType(object, 0, NULL, 0);
}

static cell_t N_SRCWRJSON_IsArray(IPluginContext* ctx, const cell_t* params)
{
	void* object;
	GET_HANDLE(params[1], object, g_JSONType);
	int flags = params[2];
	char* key;
	(void)ctx->LocalToStringNULL(params[3], &key);
	MAYBE_FORMAT(3, key);
	return rust_SRCWRJSON_IsArray(object, flags, key, fmtlen);
}

static cell_t N_SRCWRJSON_len(IPluginContext* ctx, const cell_t* params)
{
	void* object;
	GET_HANDLE(params[1], object, g_JSONType);
	int flags = params[2];
	char* key;
	(void)ctx->LocalToStringNULL(params[3], &key);
	MAYBE_FORMAT(3, key);
	return rust_SRCWRJSON_len(object, flags, key, fmtlen);
}

static cell_t N_SRCWRJSON_Length_get(IPluginContext* ctx, const cell_t* params)
{
	void* object;
	GET_HANDLE(params[1], object, g_JSONType);
	return rust_SRCWRJSON_len(object, 0, NULL, 0);
}

static cell_t N_SRCWRJSON_Get(IPluginContext* ctx, const cell_t* params)
{
	void* object;
	GET_HANDLE(params[1], object, g_JSONType);
	int flags = params[2];
	char* key;
	(void)ctx->LocalToStringNULL(params[3], &key);
	MAYBE_FORMAT(3, key);
	return g_MyExtension.HandleOrDestroy(ctx, rust_SRCWRJSON_Get(object, flags, key, fmtlen), g_JSONType);
}

static cell_t N_SRCWRJSON_GetIdx(IPluginContext* ctx, const cell_t* params)
{
	void* object;
	GET_HANDLE(params[1], object, g_JSONType);
	int idx = params[2];
	int flags = params[3];
	return g_MyExtension.HandleOrDestroy(ctx, rust_SRCWRJSON_GetIdx(object, flags, idx), g_JSONType);
}

static cell_t N_SRCWRJSON_Set(IPluginContext* ctx, const cell_t* params)
{
	void* object;
	GET_HANDLE(params[1], object, g_JSONType);
	void* other;
	GET_HANDLE(params[2], other, g_JSONType);
	int flags = params[3];
	char* key;
	(void)ctx->LocalToStringNULL(params[4], &key);
	MAYBE_FORMAT(4, key);
	return rust_SRCWRJSON_Set(object, other, flags, key, fmtlen);
}

static cell_t N_SRCWRJSON_SetIdx(IPluginContext* ctx, const cell_t* params)
{
	void* object;
	GET_HANDLE(params[1], object, g_JSONType);
	void* other;
	GET_HANDLE(params[2], other, g_JSONType);
	int flags = params[3];
	int idx = params[4];
	return rust_SRCWRJSON_SetIdx(object, other, flags, idx);
}

static cell_t N_SRCWRJSON_SetFromString(IPluginContext* ctx, const cell_t* params)
{
	void* object;
	GET_HANDLE(params[1], object, g_JSONType);
	char* s;
	(void)ctx->LocalToString(params[2], &s);
	int end = params[3];
	int flags = params[4];
	char* key;
	(void)ctx->LocalToStringNULL(params[5], &key);
	MAYBE_FORMAT(5, key);
	return rust_SRCWRJSON_SetFromString(object, s, end, flags, key, fmtlen);
}

static cell_t N_SRCWRJSON_SetFromStringIdx(IPluginContext* ctx, const cell_t* params)
{
	void* object;
	GET_HANDLE(params[1], object, g_JSONType);
	char* s;
	(void)ctx->LocalToString(params[2], &s);
	int end = params[3];
	int flags = params[4];
	int idx = params[5];
	return rust_SRCWRJSON_SetFromStringIdx(object, s, end, flags, idx);
}

static cell_t N_SRCWRJSON_Remove(IPluginContext* ctx, const cell_t* params)
{
	void* object;
	GET_HANDLE(params[1], object, g_JSONType);
	int flags = params[2];
	char* key;
	(void)ctx->LocalToString(params[3], &key);
	MAYBE_FORMAT(3, key);
	return rust_SRCWRJSON_Remove(object, flags, key, fmtlen);
}

static cell_t N_SRCWRJSON_RemoveIdx(IPluginContext* ctx, const cell_t* params)
{
	void* object;
	GET_HANDLE(params[1], object, g_JSONType);
	int flags = params[2];
	int idx = params[3];
	return rust_SRCWRJSON_RemoveIdx(object, flags, idx);
}

static cell_t N_SRCWRJSON_RemoveAllWithSelector(IPluginContext* ctx, const cell_t* params)
{
	void* object;
	GET_HANDLE(params[1], object, g_JSONType);
	char* selectorpath;
	(void)ctx->LocalToString(params[2], &selectorpath);
	int flags = params[3];
	char* key;
	(void)ctx->LocalToStringNULL(params[4], &key);
	MAYBE_FORMAT(4, key);
	return rust_SRCWRJSON_RemoveAllWithSelector(object, selectorpath, flags, key, fmtlen);
}

static cell_t N_SRCWRJSON_Clear(IPluginContext* ctx, const cell_t* params)
{
	void* object;
	GET_HANDLE(params[1], object, g_JSONType);
	int flags = params[2];
	char* key;
	(void)ctx->LocalToStringNULL(params[3], &key);
	MAYBE_FORMAT(3, key);
	return rust_SRCWRJSON_Clear(object, flags, key, fmtlen);
}

static cell_t N_SRCWRJSON_ReplaceCell(IPluginContext* ctx, const cell_t* params)
{
	void* object;
	GET_HANDLE(params[1], object, g_JSONType);
	int newcell = params[2];
	int flags = params[3];
	char* key;
	(void)ctx->LocalToStringNULL(params[4], &key);
	MAYBE_FORMAT(4, key);
	return rust_SRCWRJSON_ReplaceCell(object, newcell, flags, key, fmtlen);
}

static cell_t N_SRCWRJSON_ReplaceCellIdx(IPluginContext* ctx, const cell_t* params)
{
	void* object;
	GET_HANDLE(params[1], object, g_JSONType);
	int newcell = params[2];
	int flags = params[3];
	int idx = params[4];
	return rust_SRCWRJSON_ReplaceCellIdx(object, newcell, flags, idx);
}

static cell_t N_SRCWRJSON_SetZss(IPluginContext* ctx, const cell_t* params)
{
	void* object;
	GET_HANDLE(params[1], object, g_JSONType);
	int flags = params[2];
	char* key;
	(void)ctx->LocalToString(params[3], &key);
	void* other;
	GET_HANDLE(params[4], other, g_JSONType);
	char* key2;
	(void)ctx->LocalToString(params[5], &key2);
	MAYBE_FORMAT(5, key2);
	return rust_SRCWRJSON_SetZss(object, flags, key, other, key2, fmtlen);
}

static cell_t N_SRCWRJSON_GetStruct(IPluginContext* ctx, const cell_t* params)
{
	void* object;
	GET_HANDLE(params[1], object, g_JSONType);
	unsigned* buf;
	(void)ctx->LocalToPhysAddr(params[2], (cell_t**)&buf);
	char* format;
	(void)ctx->LocalToString(params[3], &format);
	int flags = params[4];
	char* key;
	(void)ctx->LocalToStringNULL(params[5], &key);
	MAYBE_FORMAT(5, key);
	return rust_SRCWRJSON_GetStruct(object, buf, format, flags, key, fmtlen);
}

static cell_t N_SRCWRJSON_GetStructIdx(IPluginContext* ctx, const cell_t* params)
{
	void* object;
	GET_HANDLE(params[1], object, g_JSONType);
	unsigned* buf;
	(void)ctx->LocalToPhysAddr(params[2], (cell_t**)&buf);
	char* format;
	(void)ctx->LocalToString(params[3], &format);
	int flags = params[4];
	int idx = params[5];
	return rust_SRCWRJSON_GetStructIdx(object, buf, format, flags, idx);
}

static cell_t N_SRCWRJSON_SetStruct(IPluginContext* ctx, const cell_t* params)
{
	void* object;
	GET_HANDLE(params[1], object, g_JSONType);
	unsigned* buf;
	(void)ctx->LocalToPhysAddr(params[2], (cell_t**)&buf);
	char* format;
	(void)ctx->LocalToString(params[3], &format);
	int flags = params[4];
	char* key;
	(void)ctx->LocalToStringNULL(params[5], &key);
	MAYBE_FORMAT(5, key);
	return rust_SRCWRJSON_SetStruct(object, buf, format, flags, key, fmtlen);
}

static cell_t N_SRCWRJSON_SetStructIdx(IPluginContext* ctx, const cell_t* params)
{
	void* object;
	GET_HANDLE(params[1], object, g_JSONType);
	unsigned* buf;
	(void)ctx->LocalToPhysAddr(params[2], (cell_t**)&buf);
	char* format;
	(void)ctx->LocalToString(params[3], &format);
	int flags = params[4];
	int idx = params[5];
	return rust_SRCWRJSON_SetStructIdx(object, buf, format, flags, idx);
}

static cell_t N_SRCWRJSON_GetCell(IPluginContext* ctx, const cell_t* params)
{
	void* object;
	GET_HANDLE(params[1], object, g_JSONType);
	int flags = params[2];
	char* key;
	(void)ctx->LocalToString(params[3], &key);
	MAYBE_FORMAT(3, key);
	return rust_SRCWRJSON_GetCell(object, flags, key, fmtlen);
}

static cell_t N_SRCWRJSON_GetCellIdx(IPluginContext* ctx, const cell_t* params)
{
	void* object;
	GET_HANDLE(params[1], object, g_JSONType);
	int flags = params[2];
	int idx = params[3];
	return rust_SRCWRJSON_GetCellIdx(object, flags, idx);
}

static cell_t N_SRCWRJSON_SetCell(IPluginContext* ctx, const cell_t* params)
{
	void* object;
	GET_HANDLE(params[1], object, g_JSONType);
	int value = params[2];
	int flags = params[3];
	char* key;
	(void)ctx->LocalToString(params[4], &key);
	MAYBE_FORMAT(4, key);
	return rust_SRCWRJSON_SetCell(object, value, flags, key, fmtlen);
}

static cell_t N_SRCWRJSON_SetCellIdx(IPluginContext* ctx, const cell_t* params)
{
	void* object;
	GET_HANDLE(params[1], object, g_JSONType);
	int value = params[2];
	int flags = params[3];
	int idx = params[4];
	return rust_SRCWRJSON_SetCellIdx(object, value, flags, idx);
}

static cell_t N_SRCWRJSON_GetF32(IPluginContext* ctx, const cell_t* params)
{
	void* object;
	GET_HANDLE(params[1], object, g_JSONType);
	int flags = params[2];
	char* key;
	(void)ctx->LocalToString(params[3], &key);
	MAYBE_FORMAT(3, key);
	return rust_SRCWRJSON_GetF32(object, flags, key, fmtlen);
}

static cell_t N_SRCWRJSON_GetF32Idx(IPluginContext* ctx, const cell_t* params)
{
	void* object;
	GET_HANDLE(params[1], object, g_JSONType);
	int flags = params[2];
	int idx = params[3];
	return rust_SRCWRJSON_GetF32Idx(object, flags, idx);
}

static cell_t N_SRCWRJSON_SetF32(IPluginContext* ctx, const cell_t* params)
{
	void* object;
	GET_HANDLE(params[1], object, g_JSONType);
	float value = sp_ctof(params[2]);
	int flags = params[3];
	char* key;
	(void)ctx->LocalToString(params[4], &key);
	MAYBE_FORMAT(4, key);
	return rust_SRCWRJSON_SetF32(object, value, flags, key, fmtlen);
}

static cell_t N_SRCWRJSON_SetF32Idx(IPluginContext* ctx, const cell_t* params)
{
	void* object;
	GET_HANDLE(params[1], object, g_JSONType);
	float value = sp_ctof(params[2]);
	int flags = params[3];
	int idx = params[4];
	return rust_SRCWRJSON_SetF32Idx(object, value, flags, idx);
}

static cell_t N_SRCWRJSON_GetBool(IPluginContext* ctx, const cell_t* params)
{
	void* object;
	GET_HANDLE(params[1], object, g_JSONType);
	int flags = params[2];
	char* key;
	(void)ctx->LocalToString(params[3], &key);
	MAYBE_FORMAT(3, key);
	return rust_SRCWRJSON_GetBool(object, flags, key, fmtlen);
}

static cell_t N_SRCWRJSON_GetBoolIdx(IPluginContext* ctx, const cell_t* params)
{
	void* object;
	GET_HANDLE(params[1], object, g_JSONType);
	int flags = params[2];
	int idx = params[3];
	return rust_SRCWRJSON_GetBoolIdx(object, flags, idx);
}

static cell_t N_SRCWRJSON_SetBool(IPluginContext* ctx, const cell_t* params)
{
	void* object;
	GET_HANDLE(params[1], object, g_JSONType);
	bool value = params[2] != 0;
	int flags = params[3];
	char* key;
	(void)ctx->LocalToString(params[4], &key);
	MAYBE_FORMAT(4, key);
	return rust_SRCWRJSON_SetBool(object, value, flags, key, fmtlen);
}

static cell_t N_SRCWRJSON_SetBoolIdx(IPluginContext* ctx, const cell_t* params)
{
	void* object;
	GET_HANDLE(params[1], object, g_JSONType);
	bool value = params[2] != 0;
	int flags = params[3];
	int idx = params[4];
	return rust_SRCWRJSON_SetBoolIdx(object, value, flags, idx);
}

static cell_t N_SRCWRJSON_ToggleBool(IPluginContext* ctx, const cell_t* params)
{
	void* object;
	GET_HANDLE(params[1], object, g_JSONType);
	int flags = params[2];
	char* key;
	(void)ctx->LocalToString(params[3], &key);
	MAYBE_FORMAT(3, key);
	return rust_SRCWRJSON_ToggleBool(object, flags, key, fmtlen);
}

static cell_t N_SRCWRJSON_ToggleBoolIdx(IPluginContext* ctx, const cell_t* params)
{
	void* object;
	GET_HANDLE(params[1], object, g_JSONType);
	int flags = params[2];
	int idx = params[3];
	return rust_SRCWRJSON_ToggleBoolIdx(object, flags, idx);
}

static cell_t N_SRCWRJSON_GetI64(IPluginContext* ctx, const cell_t* params)
{
	void* object;
	GET_HANDLE(params[1], object, g_JSONType);
	int64_t* buffer;
	(void)ctx->LocalToPhysAddr(params[2], (cell_t**)&buffer);
	int flags = params[3];
	char* key;
	(void)ctx->LocalToString(params[4], &key);
	MAYBE_FORMAT(4, key);
	return rust_SRCWRJSON_GetI64(object, buffer, flags, key, fmtlen);
}

static cell_t N_SRCWRJSON_GetI64Idx(IPluginContext* ctx, const cell_t* params)
{
	void* object;
	GET_HANDLE(params[1], object, g_JSONType);
	int64_t* buffer;
	(void)ctx->LocalToPhysAddr(params[2], (cell_t**)&buffer);
	int flags = params[3];
	int idx = params[4];
	return rust_SRCWRJSON_GetI64Idx(object, buffer, flags, idx);
}

static cell_t N_SRCWRJSON_SetI64(IPluginContext* ctx, const cell_t* params)
{
	void* object;
	GET_HANDLE(params[1], object, g_JSONType);
	int64_t* value;
	(void)ctx->LocalToPhysAddr(params[2], (cell_t**)&value);
	int flags = params[3];
	char* key;
	(void)ctx->LocalToString(params[4], &key);
	MAYBE_FORMAT(4, key);
	return rust_SRCWRJSON_SetI64(object, *value, flags, key, fmtlen);
}

static cell_t N_SRCWRJSON_SetI64Idx(IPluginContext* ctx, const cell_t* params)
{
	void* object;
	GET_HANDLE(params[1], object, g_JSONType);
	int64_t* value;
	(void)ctx->LocalToPhysAddr(params[2], (cell_t**)&value);
	int flags = params[3];
	int idx = params[4];
	return rust_SRCWRJSON_SetI64Idx(object, *value, flags, idx);
}

static cell_t N_SRCWRJSON_GetF64(IPluginContext* ctx, const cell_t* params)
{
	void* object;
	GET_HANDLE(params[1], object, g_JSONType);
	double* buffer;
	(void)ctx->LocalToPhysAddr(params[2], (cell_t**)&buffer);
	int flags = params[3];
	char* key;
	(void)ctx->LocalToString(params[4], &key);
	MAYBE_FORMAT(4, key);
	return rust_SRCWRJSON_GetF64(object, buffer, flags, key, fmtlen);
}

static cell_t N_SRCWRJSON_GetF64Idx(IPluginContext* ctx, const cell_t* params)
{
	void* object;
	GET_HANDLE(params[1], object, g_JSONType);
	double* buffer;
	(void)ctx->LocalToPhysAddr(params[2], (cell_t**)&buffer);
	int flags = params[3];
	int idx = params[4];
	return rust_SRCWRJSON_GetF64Idx(object, buffer, flags, idx);
}

static cell_t N_SRCWRJSON_SetF64(IPluginContext* ctx, const cell_t* params)
{
	void* object;
	GET_HANDLE(params[1], object, g_JSONType);
	double* value;
	(void)ctx->LocalToPhysAddr(params[2], (cell_t**)&value);
	int flags = params[3];
	char* key;
	(void)ctx->LocalToString(params[4], &key);
	MAYBE_FORMAT(4, key);
	return rust_SRCWRJSON_SetF64(object, *value, flags, key, fmtlen);
}

static cell_t N_SRCWRJSON_SetF64Idx(IPluginContext* ctx, const cell_t* params)
{
	void* object;
	GET_HANDLE(params[1], object, g_JSONType);
	double* value;
	(void)ctx->LocalToPhysAddr(params[2], (cell_t**)&value);
	int flags = params[3];
	int idx = params[4];
	return rust_SRCWRJSON_SetF64Idx(object, *value, flags, idx);
}

static cell_t N_SRCWRJSON_IsNull(IPluginContext* ctx, const cell_t* params)
{
	void* object;
	GET_HANDLE(params[1], object, g_JSONType);
	int flags = params[2];
	char* key;
	(void)ctx->LocalToString(params[3], &key);
	MAYBE_FORMAT(3, key);
	return rust_SRCWRJSON_IsNull(object, flags, key, fmtlen);
}

static cell_t N_SRCWRJSON_IsNullIdx(IPluginContext* ctx, const cell_t* params)
{
	void* object;
	GET_HANDLE(params[1], object, g_JSONType);
	int flags = params[2];
	int idx = params[3];
	return rust_SRCWRJSON_IsNullIdx(object, flags, idx);
}

static cell_t N_SRCWRJSON_SetNull(IPluginContext* ctx, const cell_t* params)
{
	void* object;
	GET_HANDLE(params[1], object, g_JSONType);
	int flags = params[2];
	char* key;
	(void)ctx->LocalToString(params[3], &key);
	MAYBE_FORMAT(3, key);
	return rust_SRCWRJSON_SetNull(object, flags, key, fmtlen);
}

static cell_t N_SRCWRJSON_SetNullIdx(IPluginContext* ctx, const cell_t* params)
{
	void* object;
	GET_HANDLE(params[1], object, g_JSONType);
	int flags = params[2];
	int idx = params[3];
	return rust_SRCWRJSON_SetNullIdx(object, flags, idx);
}

static cell_t N_SRCWRJSON_GetString(IPluginContext* ctx, const cell_t* params)
{
	void* object;
	GET_HANDLE(params[1], object, g_JSONType);
	char* buffer;
	(void)ctx->LocalToString(params[2], &buffer);
	cell_t buffer_local_addr = params[2]; // [sic]
	int maxlength = params[3];
	int flags = params[4];
	char* key;
	(void)ctx->LocalToString(params[5], &key);
	MAYBE_FORMAT(5, key);
	return rust_SRCWRJSON_GetString(ctx, object, buffer, buffer_local_addr, maxlength, flags, key, fmtlen);
}

static cell_t N_SRCWRJSON_GetStringIdx(IPluginContext* ctx, const cell_t* params)
{
	void* object;
	GET_HANDLE(params[1], object, g_JSONType);
	char* buffer;
	(void)ctx->LocalToString(params[2], &buffer);
	cell_t buffer_local_addr = params[2]; // [sic]
	int maxlength = params[3];
	int flags = params[4];
	int idx = params[5];
	return rust_SRCWRJSON_GetStringIdx(ctx, object, buffer, buffer_local_addr, maxlength, flags, idx);
}

static cell_t N_SRCWRJSON_SetString(IPluginContext* ctx, const cell_t* params)
{
	void* object;
	GET_HANDLE(params[1], object, g_JSONType);
	char* value;
	(void)ctx->LocalToString(params[2], &value);
	int end = params[3];
	int flags = params[4];
	char* key;
	(void)ctx->LocalToString(params[5], &key);
	MAYBE_FORMAT(5, key);
	return rust_SRCWRJSON_SetString(object, value, end, flags, key, fmtlen);
}

static cell_t N_SRCWRJSON_SetStringIdx(IPluginContext* ctx, const cell_t* params)
{
	void* object;
	GET_HANDLE(params[1], object, g_JSONType);
	char* value;
	(void)ctx->LocalToString(params[2], &value);
	int end = params[3];
	int flags = params[4];
	int idx = params[5];
	return rust_SRCWRJSON_SetStringIdx(object, value, end, flags, idx);
}

static cell_t N_SRCWRJSON_FillKeys(IPluginContext* ctx, const cell_t* params)
{
	void* object;
	GET_HANDLE(params[1], object, g_JSONType);
	void* cellarray;
	HandleError err;
	if ((err = ReadHandleCoreIdent(params[2], g_ArrayListType, &cellarray))
		!= HandleError_None) [[unlikely]]
	{
		ctx->ReportError("Invalid ArrayList handle %x (error: %d)", params[2], err);
		return 0;
	}
	int flags = params[3];
	char* key;
	(void)ctx->LocalToStringNULL(params[4], &key);
	MAYBE_FORMAT(4, key);
	return rust_SRCWRJSON_FillKeys(object, cellarray, flags, key, fmtlen);
}

#define xxx(Thing) {"SRCWRJSON."#Thing, N_SRCWRJSON_ ## Thing}
extern const sp_nativeinfo_t JSONNatives[] = {
	xxx(SRCWRJSON),
	xxx(Clone),

	xxx(GetHandles),

	xxx(ToFile),
	xxx(ToFileHandle),

	xxx(ToString),

	xxx(FromFile),
	xxx(FromFileHandle),
	xxx(FromString1),
	xxx(FromString2),

	xxx(Has),
	xxx(GetType),
	{"SRCWRJSON.Type.get", N_SRCWRJSON_Type_get},
	xxx(IsArray),

	xxx(len),
	{"SRCWRJSON.Length.get", N_SRCWRJSON_Length_get},

	xxx(Get),
	xxx(GetIdx),
	xxx(Set),
	xxx(SetIdx),

	xxx(SetFromString),
	xxx(SetFromStringIdx),

	xxx(Remove),
	xxx(RemoveIdx),

	xxx(Clear),

	xxx(ReplaceCell),
	xxx(ReplaceCellIdx),

	xxx(SetZss),

	xxx(GetStruct),
	xxx(GetStructIdx),
	xxx(SetStruct),
	xxx(SetStructIdx),

	xxx(GetCell),
	xxx(GetCellIdx),
	xxx(SetCell),
	xxx(SetCellIdx),

	xxx(GetF32),
	xxx(GetF32Idx),
	xxx(SetF32),
	xxx(SetF32Idx),

	xxx(GetBool),
	xxx(GetBoolIdx),
	xxx(SetBool),
	xxx(SetBoolIdx),
	xxx(ToggleBool),
	xxx(ToggleBoolIdx),

	xxx(GetI64),
	xxx(GetI64Idx),
	xxx(SetI64),
	xxx(SetI64Idx),

	xxx(GetF64),
	xxx(GetF64Idx),
	xxx(SetF64),
	xxx(SetF64Idx),

	xxx(IsNull),
	xxx(IsNullIdx),
	xxx(SetNull),
	xxx(SetNullIdx),

	xxx(GetString),
	xxx(GetStringIdx),
	xxx(SetString),
	xxx(SetStringIdx),

	xxx(FillKeys),

	{NULL, NULL}
};
