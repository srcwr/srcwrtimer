// SPDX-License-Identifier: GPL-3.0-or-later
// Copyright 2022 rtldg <rtldg@protonmail.com>
// This file is part of srcwrtimer (https://github.com/srcwr/srcwrtimer/)

#include "../../extshared/src/extension.h"
#include "../../extshared/src/coreident.hpp"
#include <logic/sprintf.h>
#include "rust_exports_http.h"
#include "../../srcwrjson/src/ISRCWRJSONHello.hpp"

HandleType_t g_HTTPReqType = 0;
HandleType_t g_HTTPRespType = 0;
HandleType_t g_WebsocketType = 0;
HandleType_t g_WebsocketMsgType = 0;
ISRCWRJSONHello* g_ISRCWRJSONHello;

extern const sp_nativeinfo_t HTTPNatives[];

class HandleHandler : public IHandleTypeDispatch
{
public:
	void OnHandleDestroy(HandleType_t type, void* object)
	{
		if (type == g_HTTPReqType)
			rust_handle_destroy_SRCWRHTTPReq(object);
		else if (type == g_HTTPRespType)
			rust_handle_destroy_SRCWRHTTPResp(object);
		else if (type == g_WebsocketType)
			rust_handle_destroy_SRCWRWebsocket(object);
		else if (type == g_WebsocketMsgType)
			rust_handle_destroy_SRCWRWebsocketMsg(object);
	}
	bool GetHandleApproxSize(HandleType_t type, void* object, unsigned int* size)
	{
		if (type == g_HTTPReqType)
			return rust_handle_size_SRCWRHTTPReq(object, size);
		else if (type == g_HTTPRespType)
			return rust_handle_size_SRCWRHTTPResp(object, size);
		else if (type == g_WebsocketType)
			return rust_handle_size_SRCWRWebsocket(object, size);
		else if (type == g_WebsocketMsgType)
			return rust_handle_size_SRCWRWebsocketMsg(object, size);
		return false;
	}
};

HandleHandler g_HandleHandler;

void Frame_RetryJSONHello(bool simulating)
{
	if (!g_ISRCWRJSONHello)
		sharesys->RequestInterface(SM_MKIFACE(JSONHELLO), myself, (SMInterface**)&g_ISRCWRJSONHello);
}

bool Extension_OnLoad(char* error, size_t maxlength)
{
	MyQueryInterfaceDrop = [](SMInterface* pInterface) -> bool {
		if (pInterface == (SMInterface*)g_ISRCWRJSONHello)
			return true;
		return false;
	};

	MyNotifyInterfaceDrop = [](SMInterface* interface) {
		if (interface == (SMInterface*)g_ISRCWRJSONHello)
			g_ISRCWRJSONHello = nullptr;
	};

	// require = false, autoload = true
	sharesys->AddDependency(myself, "srcwrjson.ext", false, true);
	Frame_RetryJSONHello(false);
	smutils->AddGameFrameHook(Frame_RetryJSONHello);

	sharesys->AddNatives(myself, HTTPNatives);

	// Setup access so plugins can't clone handles.
	// So they don't fuck up my precious memory...
	HandleAccess noclone{};
	noclone.access[HandleAccess_Clone] = HANDLE_RESTRICT_OWNER|HANDLE_RESTRICT_IDENTITY;
	noclone.access[HandleAccess_Delete] = 0;
	noclone.access[HandleAccess_Read] = HANDLE_RESTRICT_IDENTITY;

	g_HTTPReqType = g_pHandleSys->CreateType(
		  "SRCWRHTTPReq"
		, &g_HandleHandler
		, 0
		, NULL
		, &noclone
		, myself->GetIdentity()
		, NULL
	);

	g_HTTPRespType = g_pHandleSys->CreateType(
		  "SRCWRHTTPResp"
		, &g_HandleHandler
		, 0
		, NULL
		, NULL // can clone
		, myself->GetIdentity()
		, NULL
	);

	g_WebsocketType = g_pHandleSys->CreateType(
		  "SRCWRHTTPWebsocket"
		, &g_HandleHandler
		, 0
		, NULL
		, &noclone
		, myself->GetIdentity()
		, NULL
	);

	g_WebsocketMsgType = g_pHandleSys->CreateType(
		  "SRCWRHTTPWebsocketMsg"
		, &g_HandleHandler
		, 0
		, NULL
		, NULL // can clone
		, myself->GetIdentity()
		, NULL
	);

	return true;
}

void Extension_OnUnload()
{
	smutils->RemoveGameFrameHook(Frame_RetryJSONHello);
	g_pHandleSys->RemoveType(g_HTTPReqType, myself->GetIdentity());
	g_pHandleSys->RemoveType(g_HTTPRespType, myself->GetIdentity());
}

void Extension_OnAllLoaded() {}

void* quickhandlecheck(IPluginContext* ctx, cell_t param, HandleType_t htype)
{
	HandleError err;
	HandleSecurity sec(ctx->GetIdentity(), myself->GetIdentity());
	void* object;

	if ((err = handlesys->ReadHandle(param, htype, &sec, &object))
	    != HandleError_None) [[unlikely]]
	{
		ctx->ReportError("Invalid handle %x (error: %d)", param, err);
		return NULL;
	}

	return object;
}
#define QUICK_HANDLE_CHECK(param) if (!(object = quickhandlecheck(ctx, param, g_HTTPReqType))) return 0;
#define QUICK_HANDLE_CHECK_RESP(param) if (!(object = quickhandlecheck(ctx, param, g_HTTPRespType))) return 0;
#define QUICK_HANDLE_CHECK_WS(param) if (!(object = quickhandlecheck(ctx, param, g_WebsocketType))) return 0;
#define QUICK_HANDLE_CHECK_WSMSG(param) if (!(object = quickhandlecheck(ctx, param, g_WebsocketMsgType))) return 0;

cell_t HandleOrDestroy(IPluginContext* ctx, void* object, HandleType_t htype)
{
	if (!object) return 0;
	auto handle = 0;

	if (ctx && htype != g_HTTPReqType)
	{
		handle = handlesys->CreateHandle(
			  htype
			, object
			, ctx->GetIdentity()
			, myself->GetIdentity()
			, NULL
		);
	}
	else
	{
		HandleSecurity sec(NULL, myself->GetIdentity());
		handle = handlesys->CreateHandleEx(
			  htype
			, object
			, &sec
			, NULL
			, NULL
		);
	}

	// printf("Created %s handle (0x%X) / (0x%X)\n", isresp ? "response" : "REQ", handle, object);

	if (handle == 0) [[unlikely]]
		g_HandleHandler.OnHandleDestroy(htype, object);

	return handle;
}

PLATFORM_EXTERN_C void cpp_forward_http_resp(
	  IChangeableForward* fw
	, cell_t user_value
	, const char* error
	, void* object
	, const char* download_path
)
{
	Handle_t handle = 0;
	HandleSecurity sec(NULL, myself->GetIdentity());

	if (!download_path)
	{
		if (0 == (handle = HandleOrDestroy(NULL, object, g_HTTPRespType)))
		{
			error = "Couldn't create SRCWRHTTPResp handle????";
		}
	}

	fw->PushCell(user_value);
	fw->PushString(error);
	if (download_path)
		fw->PushString(download_path);
	else
		fw->PushCell(handle);
	int _ret = fw->Execute(NULL);
	forwards->ReleaseForward(fw);

	if (handle)
		handlesys->FreeHandle(handle, &sec);
}

PLATFORM_EXTERN_C void cpp_forward_websocket_msg(
	  IChangeableForward* fw
	, cell_t wshandle
	, cell_t user_value
	, const char* close_reason
	, void* msgobject
)
{
	Handle_t msghandle = 0;
	HandleSecurity sec(NULL, myself->GetIdentity());

	if (msgobject && 0 == (msghandle = HandleOrDestroy(NULL, msgobject, g_WebsocketMsgType)))
	{
		// How are we here? How is it a bad thing to be here? I followed a `TODO: ??` message here and I wish I explained wtf the problem was...
		return;
	}

	fw->PushCell(user_value);
	fw->PushCell(wshandle);
	if (msghandle)
		fw->PushCell(msghandle);
	else
		fw->PushString(close_reason ? close_reason : "");
	int _ret = fw->Execute(NULL);

	if (msghandle)
		handlesys->FreeHandle(msghandle, &sec);
	// else if (close_reason && close_reason[0] != '\0')
	// 	handlesys->FreeHandle(wshandle, &sec);
}

static cell_t N_SRCWRHTTPReq_SRCWRHTTPReq(IPluginContext* ctx, const cell_t* params)
{
	COMPAT_CHECK("srcwrhttp_compat_version", 1);

	char* url;
	(void)ctx->LocalToString(params[1], &url);
	MAYBE_FORMAT(1, url);
	if (url[0] == '\0')
	{
		ctx->ReportError("URL is an empty string");
		return 0;
	}
	return HandleOrDestroy(ctx, rust_SRCWRHTTPReq_SRCWRHTTPReq(url), g_HTTPReqType);
}

static cell_t N_SRCWRHTTPReq_YEET(IPluginContext* ctx, const cell_t* params)
{
	void* object;
	QUICK_HANDLE_CHECK(params[1]);
	cell_t callback = params[2];
	int value = params[3];
	char* method;
	(void)ctx->LocalToString(params[4], &method);

	IChangeableForward* fw = forwards->CreateForwardEx(
		  NULL
		, ET_Ignore
		, 3
		, NULL
		, Param_Any // value
		, Param_String // error
		, Param_Any // handle
	);

	auto ret = 0;

	if (fw)
	{
		if (fw->AddFunction(ctx, callback))
		{
			rust_SRCWRHTTPReq_YEET(object, fw, value, method);
			ret = 1;
		}
		else
		{
			forwards->ReleaseForward(fw);
			ctx->ReportError("Failed to add the callback to the forward...");
		}
	}
	else
	{
		ctx->ReportError("Failed to create a forward...");
	}

	HandleSecurity sec(ctx->GetIdentity(), myself->GetIdentity());
	handlesys->FreeHandle(params[1], &sec);

	return ret;
}

static cell_t N_SRCWRHTTPReq_Download(IPluginContext* ctx, const cell_t* params)
{
	void* object;
	QUICK_HANDLE_CHECK(params[1]);
	int callback = params[2];
	int value = params[3];
	char* sp_filename;
	(void)ctx->LocalToString(params[4], &sp_filename);
	MAYBE_FORMAT(4, sp_filename);
	if (sp_filename[0] == '\0')
	{
		ctx->ReportError("Filename is an empty string");
		return 0;
	}
	char real_filename[PLATFORM_MAX_PATH];
	smutils->BuildPath(Path_Game, real_filename, sizeof(real_filename), "%s", sp_filename);

	IChangeableForward* fw = forwards->CreateForwardEx(
		  NULL
		, ET_Ignore
		, 3
		, NULL
		, Param_Any // value
		, Param_String // error
		, Param_String // filename
	);

	auto ret = 0;

	if (fw)
	{
		if (fw->AddFunction(ctx, callback))
		{
			ret = rust_SRCWRHTTPReq_Download(object, fw, value, sp_filename, real_filename);
		}
		else
		{
			forwards->ReleaseForward(fw);
			ctx->ReportError("Failed to add the callback to the forward...");
		}
	}
	else
	{
		ctx->ReportError("Failed to create a forward...");
	}

	HandleSecurity sec(ctx->GetIdentity(), myself->GetIdentity());
	handlesys->FreeHandle(params[1], &sec);

	return ret;
}

static cell_t N_SRCWRHTTPReq_method(IPluginContext* ctx, const cell_t* params)
{
	void* object;
	QUICK_HANDLE_CHECK(params[1]);
	char* method;
	(void)ctx->LocalToString(params[2], &method);
	return rust_SRCWRHTTPReq_method(object, method);
}

static cell_t N_SRCWRHTTPReq_header(IPluginContext* ctx, const cell_t* params)
{
	void* object;
	QUICK_HANDLE_CHECK(params[1]);
	char* key;
	(void)ctx->LocalToString(params[2], &key);
	char* value;
	(void)ctx->LocalToString(params[3], &value);
	MAYBE_FORMAT(3, value);
	return rust_SRCWRHTTPReq_header(object, key, value);
}

static cell_t N_SRCWRHTTPReq_query_param(IPluginContext* ctx, const cell_t* params)
{
	void* object;
	QUICK_HANDLE_CHECK(params[1]);
	char* key;
	(void)ctx->LocalToString(params[2], &key);
	char* value;
	(void)ctx->LocalToString(params[3], &value);
	MAYBE_FORMAT(3, value);
	return rust_SRCWRHTTPReq_query_param(object, key, value);
}

static cell_t N_SRCWRHTTPReq_body_set(IPluginContext* ctx, const cell_t* params)
{
	void* object;
	QUICK_HANDLE_CHECK(params[1]);
	int end = params[2];
	char* content;
	(void)ctx->LocalToString(params[3], &content);
	MAYBE_FORMAT(3, content);
	return rust_SRCWRHTTPReq_body_set(object, end, content);
}

static cell_t N_SRCWRHTTPReq_body_add_file_on_send(IPluginContext* ctx, const cell_t* params)
{
	void* object;
	QUICK_HANDLE_CHECK(params[1]);
	char* filename;
	(void)ctx->LocalToString(params[2], &filename);
	MAYBE_FORMAT(2, filename);
	char filenamebuf[PLATFORM_MAX_PATH];
	smutils->BuildPath(Path_Game, filenamebuf, sizeof(filenamebuf), "%s", filename);
	return rust_SRCWRHTTPReq_body_add_file_on_send(object, filenamebuf);
}

static cell_t N_SRCWRHTTPReq_body_add(IPluginContext* ctx, const cell_t* params)
{
	void* object;
	QUICK_HANDLE_CHECK(params[1]);
	int end = params[2];
	char* content;
	(void)ctx->LocalToString(params[3], &content);
	MAYBE_FORMAT(3, content);
	return rust_SRCWRHTTPReq_body_add(object, end, content);
}

static cell_t N_SRCWRHTTPReq_body_add_file(IPluginContext* ctx, const cell_t* params)
{
	void* object;
	QUICK_HANDLE_CHECK(params[1]);
	char* filename;
	(void)ctx->LocalToString(params[2], &filename);
	MAYBE_FORMAT(2, filename);
	char filenamebuf[PLATFORM_MAX_PATH];
	smutils->BuildPath(Path_Game, filenamebuf, sizeof(filenamebuf), "%s", filename);
	return rust_SRCWRHTTPReq_body_add_file(object, filenamebuf);
}

static cell_t N_SRCWRHTTPReq_body_add_file_handle(IPluginContext* ctx, const cell_t* params)
{
	void* object;
	QUICK_HANDLE_CHECK(params[1]);
	cell_t filehandle = params[2];

	HandleError err;
	HandleSecurity sec(g_pCoreIdent, g_pCoreIdent);
	IFileObject* fileobject;

	if ((err = handlesys->ReadHandle(filehandle, g_FileType, &sec, (void**)&fileobject))
	    != HandleError_None)
	{
		ctx->ReportError("Invalid file handle %x (error: %d)", filehandle, err);
		return 0;
	}

	return rust_SRCWRHTTPReq_body_add_file_handle(object, fileobject);
}

static cell_t N_SRCWRHTTPReq_local_address(IPluginContext* ctx, const cell_t* params)
{
	void* object;
	QUICK_HANDLE_CHECK(params[1]);
	char* addr;
	(void)ctx->LocalToString(params[2], &addr);
	return rust_SRCWRHTTPReq_local_address(object, addr);
}

static cell_t N_SRCWRHTTPReq_basic_auth(IPluginContext* ctx, const cell_t* params)
{
	void* object;
	QUICK_HANDLE_CHECK(params[1]);
	char* username;
	(void)ctx->LocalToString(params[2], &username);
	char* password;
	(void)ctx->LocalToString(params[3], &password);
	return rust_SRCWRHTTPReq_basic_auth(object, username, password);
}

static cell_t N_SRCWRHTTPReq_allow_invalid_certs(IPluginContext* ctx, const cell_t* params)
{
	void* object;
	QUICK_HANDLE_CHECK(params[1]);
	rust_SRCWRHTTPReq_allow_invalid_certs(object);
	return 1;
}

static cell_t N_SRCWRHTTPReq_max_redirections_get(IPluginContext* ctx, const cell_t* params)
{
	void* object;
	QUICK_HANDLE_CHECK(params[1]);
	return rust_SRCWRHTTPReq_max_redirections_get(object);
}

static cell_t N_SRCWRHTTPReq_timeout_get(IPluginContext* ctx, const cell_t* params)
{
	void* object;
	QUICK_HANDLE_CHECK(params[1]);
	return rust_SRCWRHTTPReq_timeout_get(object);
}

static cell_t N_SRCWRHTTPReq_connect_timeout_get(IPluginContext* ctx, const cell_t* params)
{
	void* object;
	QUICK_HANDLE_CHECK(params[1]);
	return rust_SRCWRHTTPReq_connect_timeout_get(object);
}

static cell_t N_SRCWRHTTPReq_max_redirections_set(IPluginContext* ctx, const cell_t* params)
{
	void* object;
	QUICK_HANDLE_CHECK(params[1]);
	rust_SRCWRHTTPReq_max_redirections_set(object, params[2]);
	return 1;
}

static cell_t N_SRCWRHTTPReq_timeout_set(IPluginContext* ctx, const cell_t* params)
{
	void* object;
	QUICK_HANDLE_CHECK(params[1]);
	rust_SRCWRHTTPReq_timeout_set(object, params[2]);
	return 1;
}

static cell_t N_SRCWRHTTPReq_connect_timeout_set(IPluginContext* ctx, const cell_t* params)
{
	void* object;
	QUICK_HANDLE_CHECK(params[1]);
	rust_SRCWRHTTPReq_connect_timeout_set(object, params[2]);
	return 1;
}

static cell_t N_SRCWRHTTPResp_header(IPluginContext* ctx, const cell_t* params)
{
	void* object;
	QUICK_HANDLE_CHECK_RESP(params[1]);
	char* name;
	(void)ctx->LocalToString(params[2], &name);
	char* buffer;
	(void)ctx->LocalToString(params[3], &buffer);
	int maxlength = params[4];
	return rust_SRCWRHTTPResp_header(object, name, buffer, maxlength);
}

static cell_t N_SRCWRHTTPResp_get(IPluginContext* ctx, const cell_t* params)
{
	void* object;
	QUICK_HANDLE_CHECK_RESP(params[1]);
	char* buf;
	(void)ctx->LocalToString(params[2], &buf);
	int buflen = params[3];
	int flags = params[4];
	return rust_SRCWRHTTPResp_get(object, buf, buflen, flags);
}

static cell_t N_SRCWRHTTPResp_status_get(IPluginContext* ctx, const cell_t* params)
{
	void* object;
	QUICK_HANDLE_CHECK_RESP(params[1]);
	return rust_SRCWRHTTPResp_status_get(object);
}

static cell_t N_SRCWRHTTPResp_text_length_get(IPluginContext* ctx, const cell_t* params)
{
	void* object;
	QUICK_HANDLE_CHECK_RESP(params[1]);
	return rust_SRCWRHTTPResp_text_length_get(object);
}

static cell_t N_SRCWRHTTPResp_byte_length_get(IPluginContext* ctx, const cell_t* params)
{
	void* object;
	QUICK_HANDLE_CHECK_RESP(params[1]);
	return rust_SRCWRHTTPResp_byte_length_get(object);
}

static cell_t N_SRCWRHTTPResp_json_get(IPluginContext* ctx, const cell_t* params)
{
	if (!g_ISRCWRJSONHello) return 0;
	void* object;
	QUICK_HANDLE_CHECK_RESP(params[1]);
	size_t len;
	auto s = rust_SRCWRHTTPResp_json_get_inner(object, &len);
	return s ? g_ISRCWRJSONHello->MakeJSONObject(ctx, s, len) : 0;
}

static cell_t N_SRCWRWebsocket_SRCWRWebsocket(IPluginContext* ctx, const cell_t* params)
{
	auto object = rust_SRCWRWebsocket_SRCWRWebsocket();
	auto handle = HandleOrDestroy(ctx, object, g_WebsocketType);
	if (handle) rust_SRCWRWebsocket_set_handle(object, handle);
	return handle;
}

static cell_t N_SRCWRWebsocket_header(IPluginContext* ctx, const cell_t* params)
{
	void* object;
	QUICK_HANDLE_CHECK_WS(params[1]);
	char* key;
	(void)ctx->LocalToString(params[2], &key);
	char* value;
	(void)ctx->LocalToString(params[3], &value);
	MAYBE_FORMAT(3, value);
	return rust_SRCWRWebsocket_header(object, key, value);
}

static cell_t N_SRCWRWebsocket_write_json(IPluginContext* ctx, const cell_t* params)
{
	void* object;
	QUICK_HANDLE_CHECK_WS(params[1]);
	void* json = g_ISRCWRJSONHello->GetUnsafePointer(ctx, params[2]);
	if (!json) return 0;
	return rust_SRCWRWebsocket_write_json(object, json);
}

static cell_t N_SRCWRWebsocket_write_str(IPluginContext* ctx, const cell_t* params)
{
	void* object;
	QUICK_HANDLE_CHECK_WS(params[1]);
	char* buffer;
	(void)ctx->LocalToString(params[2], &buffer);
	MAYBE_FORMAT(2, buffer);
	return rust_SRCWRWebsocket_write_str(object, buffer);
}

static cell_t N_SRCWRWebsocket_YEET(IPluginContext* ctx, const cell_t* params)
{
	void* object;
	QUICK_HANDLE_CHECK_WS(params[1]);
	cell_t connectcb = params[2];
	cell_t msgcb = params[3];
	cell_t data = params[4];
	char* url;
	(void)ctx->LocalToString(params[5], &url);
	MAYBE_FORMAT(5, url);

	IChangeableForward* connection_forward = forwards->CreateForwardEx(
		  NULL
		, ET_Ignore
		, 3
		, NULL
		, Param_Any
		, Param_Any
		, Param_String
	);

	IChangeableForward* message_forward = forwards->CreateForwardEx(
		  NULL
		, ET_Ignore
		, 3
		, NULL
		, Param_Any
		, Param_Any
		, Param_Any
	);

	if (   !connection_forward
	    || !message_forward
	    || !connection_forward->AddFunction(ctx, connectcb)
	    || !message_forward->AddFunction(ctx, msgcb)
	)
	{
		if (connection_forward) forwards->ReleaseForward(connection_forward);
		if (message_forward) forwards->ReleaseForward(message_forward);
		ctx->ReportError("Failed to create callback forwards");
		return 0;
	}

	return rust_SRCWRWebsocket_YEET(
		  object
		, url
		, data
		, connection_forward
		, message_forward
	);
}

static cell_t N_SRCWRWebsocketMsg_get(IPluginContext* ctx, const cell_t* params)
{
	void* object;
	QUICK_HANDLE_CHECK_WSMSG(params[1]);
	char* buf;
	(void)ctx->LocalToString(params[2], &buf);
	int buflen = params[3];
	int flags = params[4];
	return rust_SRCWRWebsocketMsg_get(object, buf, buflen, flags);
}

static cell_t N_SRCWRWebsocketMsg_length_get(IPluginContext* ctx, const cell_t* params)
{
	void* object;
	QUICK_HANDLE_CHECK_WSMSG(params[1]);
	return rust_SRCWRWebsocketMsg_length_get(object);
}

static cell_t N_SRCWRWebsocketMsg_json_get(IPluginContext* ctx, const cell_t* params)
{
	if (!g_ISRCWRJSONHello) return 0;
	void* object;
	QUICK_HANDLE_CHECK_WSMSG(params[1]);
	size_t len;
	auto s = rust_SRCWRWebsocketMsg_json_get_inner(object, &len);
	return g_ISRCWRJSONHello->MakeJSONObject(ctx, s, len);
}

#define xxx(Thing) {"SRCWRHTTPReq."#Thing, N_SRCWRHTTPReq_ ## Thing}
extern const sp_nativeinfo_t HTTPNatives[] = {
	xxx(SRCWRHTTPReq),

	xxx(YEET),
	xxx(Download),

	xxx(method),
	xxx(header),
	xxx(query_param),

	xxx(body_set),
	xxx(body_add_file_on_send),
	xxx(body_add),
	xxx(body_add_file),
	xxx(body_add_file_handle),
	//xxx(body_add_json),

	xxx(local_address),
	xxx(basic_auth),
	xxx(allow_invalid_certs),

	{"SRCWRHTTPReq.max_redirections.get", N_SRCWRHTTPReq_max_redirections_get},
	{"SRCWRHTTPReq.max_redirections.set", N_SRCWRHTTPReq_max_redirections_set},
	{"SRCWRHTTPReq.timeout.get", N_SRCWRHTTPReq_timeout_get},
	{"SRCWRHTTPReq.timeout.set", N_SRCWRHTTPReq_timeout_set},
	{"SRCWRHTTPReq.connect_timeout.get", N_SRCWRHTTPReq_connect_timeout_get},
	{"SRCWRHTTPReq.connect_timeout.set", N_SRCWRHTTPReq_connect_timeout_set},

	{"SRCWRHTTPResp.header", N_SRCWRHTTPResp_header},
	{"SRCWRHTTPResp.get", N_SRCWRHTTPResp_get},
	{"SRCWRHTTPResp.status.get", N_SRCWRHTTPResp_status_get},
	{"SRCWRHTTPResp.text_length.get", N_SRCWRHTTPResp_text_length_get},
	{"SRCWRHTTPResp.byte_length.get", N_SRCWRHTTPResp_byte_length_get},
	{"SRCWRHTTPResp.json.get", N_SRCWRHTTPResp_json_get},

	{"SRCWRWebsocket.SRCWRWebsocket", N_SRCWRWebsocket_SRCWRWebsocket},
	{"SRCWRWebsocket.header", N_SRCWRWebsocket_header},
	{"SRCWRWebsocket.write_json", N_SRCWRWebsocket_write_json},
	{"SRCWRWebsocket.write_str", N_SRCWRWebsocket_write_str},
	{"SRCWRWebsocket.YEET", N_SRCWRWebsocket_YEET},

	{"SRCWRWebsocketMsg.get", N_SRCWRWebsocketMsg_get},
	{"SRCWRWebsocketMsg.length.get", N_SRCWRWebsocketMsg_length_get},
	{"SRCWRWebsocketMsg.json.get", N_SRCWRWebsocketMsg_json_get},

	{NULL, NULL}
};
