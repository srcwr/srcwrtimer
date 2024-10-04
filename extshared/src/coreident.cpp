// SPDX-License-Identifier: GPL-3.0-or-later
// Copyright 2021-2024 rtldg <rtldg@protonmail.com>
// This file is part of srcwrtimer (https://github.com/srcwr/srcwrtimer/)

#include "extension.h"
#include "coreident.hpp"
//#include "../../alliedmodders/sourcemod/core/logic/HandleSys.h"

IdentityToken_t *g_pCoreIdent;

#ifdef HANDLE_FILEOBJECT
HandleType_t g_FileType;
#endif
#ifdef HANDLE_CELLARRAY
HandleType_t g_ArrayListType;
#endif

#if 1 || defined(_WIN32)
// https://github.com/alliedmodders/sourcemod/blob/b14c18ee64fc822dd6b0f5baea87226d59707d5a/core/logic/HandleSys.h#L103
struct QHandleType_Caster
{
	void *dispatch;
	unsigned int freeID;
	unsigned int children;
	TypeAccess typeSec;
};
// https://github.com/alliedmodders/sourcemod/blob/b14c18ee64fc822dd6b0f5baea87226d59707d5a/core/logic/HandleSys.h#L230
struct HandleSystem_Caster
{
	void *vtable;
	void *m_Handles;
	QHandleType_Caster *m_Types;
};
#endif

bool ResolveCoreIdent(char* error, size_t maxlength)
{
#if 1 || defined(_WIN32)
	HandleSystem_Caster *blah = (HandleSystem_Caster *)g_pHandleSys;
	// g_ArrayListType doesn't work here???
	// I really have no idea what's going on. This is terrible....
	unsigned index = 512;
	g_pCoreIdent = blah->m_Types[index].typeSec.ident;
#else
	Dl_info info;
	// memutils is from sourcemod.logic.so so we can grab the module from it.
	dladdr(memutils, &info);
	void *sourcemod_logic = dlopen(info.dli_fname, RTLD_NOW);

	if (!sourcemod_logic)
	{
		snprintf(error, maxlength, "dlopen failed on '%s'", info.dli_fname);
		return false;
	}

	// Seems like ResolveSymbol() interns the resolved address so keep it like that instead of using dlsym() I guess...
	IdentityToken_t **token = (IdentityToken_t **)memutils->ResolveSymbol(sourcemod_logic, "g_pCoreIdent");

	dlclose(sourcemod_logic);

	if (!token)
	{
		snprintf(error, maxlength, "failed to resolve symbol g_pCoreIdent");
		return false;
	}

	g_pCoreIdent = *token;
#endif

	if (!g_pCoreIdent)
	{
		snprintf(error, maxlength, "g_pCoreIdent is NULL");
		return false;
	}

#ifdef HANDLE_FILEOBJECT
	if (!g_pHandleSys->FindHandleType("File", &g_FileType))
	{
		snprintf(error, maxlength, "failed to find handle type 'File'");
		return false;
	}
#endif

#ifdef HANDLE_CELLARRAY
	if (!g_pHandleSys->FindHandleType("CellArray", &g_ArrayListType))
	{
		snprintf(error, maxlength, "failed to find handle type 'CellArray' (ArrayList)");
		return false;
	}
#endif

	return true;
}

HandleError ReadHandleCoreIdent(Handle_t handle, HandleType_t type, void** object)
{
	HandleSecurity sec(g_pCoreIdent, g_pCoreIdent);
	return handlesys->ReadHandle(handle, type, &sec, object);
}
