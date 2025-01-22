// SPDX-License-Identifier: GPL-3.0-or-later
// Copyright 2022-2025 rtldg <rtldg@protonmail.com>
// This file is part of srcwrtimer (https://github.com/srcwr/srcwrtimer/)


#include "extension.h"
#include "coreident.hpp"
#ifndef META_NO_HL2SDK
#include <filesystem.h>
#endif


MyExtension g_MyExtension;
SDKExtension *g_pExtensionIface = &g_MyExtension;

// natives.cpp from each crate provides these...
bool Extension_OnLoad(char* error, size_t maxlength);
void Extension_OnUnload();
void Extension_OnAllLoaded();

bool (*MyQueryInterfaceDrop)(SMInterface*) = NULL;
void (*MyNotifyInterfaceDrop)(SMInterface*) = NULL;

CGlobalVars* gpGlobals = NULL;
#ifndef META_NO_HL2SDK
IFileSystem *filesystem = NULL;
#endif

//////////////////////////////////////////////////////////////////////////////////////////////////
// asdf
//////////////////////////////////////////////////////////////////////////////////////////////////

bool is_plugin_compatible(IPluginContext* ctx, const char* pubvarname, cell_t required)
{
	cell_t* pubvar = get_plugin_pubvar(ctx->GetRuntime(), pubvarname);
	if (!pubvar || *pubvar != required) [[unlikely]]
	{
		ctx->ReportError("Plugin is not compatible with extension %s! Update includes and recompile it! (%s (%d) should be %d)", rust_conf_name(), pubvarname, pubvar ? *pubvar : -1, required);
		return false;
	}
	return true;
}

cell_t* get_plugin_pubvar(IPluginRuntime* rt, const char* name)
{
	uint32_t idx;
	int err = rt->FindPubvarByName(name, &idx);
	if (err != SP_ERROR_NONE) return nullptr;
	cell_t *pubvar, local_addr;
	rt->GetPubvarAddrs(idx, &local_addr, &pubvar);
	return pubvar;
}

//////////////////////////////////////////////////////////////////////////////////////////////////
// asdf
//////////////////////////////////////////////////////////////////////////////////////////////////

bool MyExtension::SDK_OnLoad(char* error, size_t maxlength, bool late)
{
	if (!ResolveCoreIdent(error, maxlength)) return false;
	if (!rust_sdk_on_load_wrapper(error, maxlength, late)) return false;
	if (!Extension_OnLoad(error, maxlength)) return false;
	sharesys->RegisterLibrary(myself, rust_conf_name());
	rootconsole->ConsolePrint(">>> hello from %s!%s", rust_conf_name(), late ? " (late!)" : "");
	return true;
}

void MyExtension::SDK_OnUnload()
{
	Extension_OnUnload();
	rust_sdk_on_unload();
}

void MyExtension::SDK_OnAllLoaded()
{
	rust_sdk_on_all_loaded();
	Extension_OnAllLoaded();
}

void MyExtension::OnCoreMapStart(edict_t* edict_list, int edict_count, int client_max)
{
	rust_on_core_map_start((void*)edict_list, edict_count, client_max);
	// TODO: We don't call an Extension_OnCoreMapStart, but we could :eyes:
}

void MyExtension::OnCoreMapEnd()
{
	rust_on_core_map_end();
	// TODO: We don't call an Extension_OnCoreMapEnd, but we could :eyes:
}

bool MyExtension::QueryInterfaceDrop(SMInterface *pInterface)
{
	if (MyQueryInterfaceDrop)
		return MyQueryInterfaceDrop(pInterface);
	return false;
}

void MyExtension::NotifyInterfaceDrop(SMInterface *pInterface)
{
	if (MyNotifyInterfaceDrop)
		MyNotifyInterfaceDrop(pInterface);
}

bool MyExtension::SDK_OnMetamodLoad(ISmmAPI *ismm, char *error, size_t maxlen, bool late)
{
#ifndef META_NO_HL2SDK
	GET_V_IFACE_CURRENT(GetFileSystemFactory, filesystem, IFileSystem, FILESYSTEM_INTERFACE_VERSION);
#endif
	gpGlobals = ismm->GetCGlobals();
	return true;
}

//////////////////////////////////////////////////////////////////////////////////////////////////
// asdf
//////////////////////////////////////////////////////////////////////////////////////////////////

const char *MyExtension::GetExtensionAuthor()
{
	return rust_conf_author();
}

const char *MyExtension::GetExtensionDateString()
{
	return SMEXT_CONF_DATESTRING;
}

const char *MyExtension::GetExtensionDescription()
{
	return rust_conf_description();
}

const char *MyExtension::GetExtensionVerString()
{
	return rust_conf_version();
}

const char *MyExtension::GetExtensionName()
{
	return rust_conf_name();
}

const char *MyExtension::GetExtensionTag()
{
	return rust_conf_logtag();
}

const char *MyExtension::GetExtensionURL()
{
	return rust_conf_url();
}

const char *MyExtension::GetLicense()
{
	return rust_conf_license();
}

//////////////////////////////////////////////////////////////////////////////////////////////////
// asdf
//////////////////////////////////////////////////////////////////////////////////////////////////

// TODO: Add helpers for Rust to call ctx->LocalToString() and stuff...
//       Then you could pass Rust functions straight into sharesys->AddNatives()...

PLATFORM_EXTERN_C void cpp_free_handle(cell_t handle)
{
	HandleSecurity sec(NULL, myself->GetIdentity());
	auto code = handlesys->FreeHandle(handle, &sec);
	if (code != 0) printf("%s >>> cpp_free_handle: code %d on handle %X\n", rust_conf_name(), code, handle);
}

PLATFORM_EXTERN_C void cpp_add_frame_action(FRAMEACTION fn, void* data)
{
	smutils->AddFrameAction(fn, data);
}

PLATFORM_EXTERN_C void cpp_add_game_frame_hook(GAME_FRAME_HOOK hook)
{
	smutils->AddGameFrameHook(hook);
}

PLATFORM_EXTERN_C void cpp_remove_game_frame_hook(GAME_FRAME_HOOK hook)
{
	smutils->RemoveGameFrameHook(hook);
}

PLATFORM_EXTERN_C void cpp_report_error(IPluginContext* ctx, const char* err)
{
	// ThrowNativeError is deprecated for some reason...
	// so use ReportError which uses SP_ERROR_USER instead of SP_ERROR_NATIVE...
	ctx->ReportError("%s", err);
}

PLATFORM_EXTERN_C void* cpp_local_to_phys_addr(IPluginContext* ctx, cell_t addr)
{
	cell_t* phys_addr = NULL;
	(void)ctx->LocalToPhysAddr(addr, &phys_addr);
	return (void*)phys_addr;
}

PLATFORM_EXTERN_C size_t cpp_string_to_local_utf8(
	IPluginContext* ctx,
	cell_t addr,
	size_t maxbytes,
	const char* source)
{
	size_t written = 0;
	(void)ctx->StringToLocalUTF8(addr, maxbytes, source, &written);
	return written;
}

//////////////////////////////////////////////////////////////////////////////////////////////////
// asdf
//////////////////////////////////////////////////////////////////////////////////////////////////

PLATFORM_EXTERN_C void cpp_forward_release(IForward* fw)
{
	forwards->ReleaseForward(fw);
}

PLATFORM_EXTERN_C unsigned cpp_forward_function_count(IForward* fw)
{
	return fw->GetFunctionCount();
}

PLATFORM_EXTERN_C int cpp_forward_push_cell(IForward* forward, cell_t c)
{
	return forward->PushCell(c);
}

PLATFORM_EXTERN_C int cpp_forward_push_string(IForward* forward, const char* s)
{
	return forward->PushString(s);
}

PLATFORM_EXTERN_C int cpp_forward_push_string_ex(IForward* forward, char* s, size_t len, int str_flags, int copy_flags)
{
	return forward->PushStringEx(s, len, str_flags, copy_flags);
}

PLATFORM_EXTERN_C int cpp_forward_execute(IForward* forward, cell_t* result)
{
	return forward->Execute(result);
}

//////////////////////////////////////////////////////////////////////////////////////////////////
// asdf
//////////////////////////////////////////////////////////////////////////////////////////////////
