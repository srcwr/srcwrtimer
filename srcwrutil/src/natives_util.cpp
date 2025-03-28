// SPDX-License-Identifier: GPL-3.0-or-later
// Copyright 2022-2025 rtldg <rtldg@protonmail.com>
// This file is part of srcwrtimer (https://github.com/srcwr/srcwrtimer/)


// Get rid of some cpptools autocomplete errors since this requires hl2sdk...
#ifndef SOURCE_ENGINE
#define SOURCE_ENGINE 6
#define SE_CSS 6
#endif

#include "../../extshared/src/extension.h"
#include "../../extshared/src/coreident.hpp"
#include "rust_exports_util.h"
#include <edict.h> // CGlobalVars
#include <CDetour/detours.h>
#include <filesystem.h>

#include <time.h> // time(), strftime(), gmtime_r()/_gmtime64_s()
#include <string>


#define TEST_STUFF 1


HandleType_t g_SmolStringListType = 0;
extern const sp_nativeinfo_t UtilNatives[];


void MyExtension::OnHandleDestroy(HandleType_t type, void* object)
{
	if (type == g_SmolStringListType)
		rust_handle_destroy_SmolStringList(object);
}
bool MyExtension::GetHandleApproxSize(HandleType_t type, void* object, unsigned int* size)
{
	if (type == g_SmolStringListType)
		return rust_handle_size_SmolStringList(object, size);
	return false;
}


IGameConfig *g_GameConfig = NULL;
CDetour *g_CDownloadListGenerator_OnResourcePrecachedFullPath_detour = NULL;
CDetour *g_CDownloadListGenerate_SetStringTable_detour = NULL;
CDetour *g_CNavMesh_Load_detour = NULL;


// Don't add .nav files to the downloadables stringtable...
DETOUR_DECL_MEMBER2(CDownloadListGenerator_OnResourcePrecachedFullPath, void, char*, full, char*, relative)
{
#if TEST_STUFF
	// static std::string mapname;
	rootconsole->ConsolePrint("%s", full);
#endif

	if (0 == strncmp(relative, "maps\\", 5))
	{
		auto len = strlen(relative);
		auto extension = &(relative[len-4]);

		if (0 == strcmp(extension, ".bsp")) {
#if TEST_STUFF
			char buffer[40+1]{};
			auto x = rust_SRCWRUTIL_GetSHA1_FilePath(full, buffer);
			printf("buffer = %s\n", buffer);
#endif
		} else if (0 == strcmp(extension, ".nav")) {
			// insert map hash here...
			return;
		}
	}

	DETOUR_MEMBER_CALL(CDownloadListGenerator_OnResourcePrecachedFullPath)(full, relative);
}

DETOUR_DECL_MEMBER1(CDownloadListGenerate_SetStringTable, void, void*, stringtable)
{
#if TEST_STUFF
	rootconsole->ConsolePrint("TEST");
#endif
	DETOUR_MEMBER_CALL(CDownloadListGenerate_SetStringTable)(stringtable);
}

// Write dummy.nav and swap the mapname to use it.
// Real map navs are pretty useless for bhop because we don't have bots moving randomly around.
// In fact we can't trust maps to NOT embed .nav files that crash. *cough* bhop_nxo_strafe *cough*
// So, fuck it: dummy.nav 24/7
DETOUR_DECL_MEMBER0(CNavMesh_Load, int)
{
#if TEST_STUFF
	filesystem->SetWarningLevel(FILESYSTEM_WARNING_REPORTALLACCESSES_READWRITE);
#endif

	if (filesystem->Size("maps/dummy.nav", "GAME") == 0)
	{
		FileHandle_t f = filesystem->Open("maps/dummy.nav", "wb", "GAME");

		if (f)
		{
			static const char dummy[205+1] = "\xCE\xFA\xED\xFE\x10\x00\x00\x00\x01\x00\x00\x00\x58\xF6\x01\x00\x01\x00\x00\x01\x01\x00\x00\x00\x01\x00\x00\x00\x00\x00\x00\x00\x00\x80\xED\xC3\x00\x00\x48\x42\xFF\x1F\x00\x42\x00\x00\x48\xC2\x00\x80\xED\x43\xFF\x1F\x00\x42\xFF\x1F\x00\x42\xFF\x1F\x00\x42\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x04\x00\x00\x00\x00\x00\x40\xE7\xC3\x00\x00\x7A\x42\xFF\x1F\x00\x42\x01\x01\x00\x00\x00\x00\x00\x7A\xC2\x00\x00\x7A\x42\xFF\x1F\x00\x42\x01\x02\x00\x00\x00\x00\x00\x7A\xC2\x00\x40\xE7\x43\xFF\x1F\x00\x42\x01\x03\x00\x00\x00\x00\x40\xE7\xC3\x00\x40\xE7\x43\xFF\x1F\x00\x42\x01\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\xF0\x42\x00\x00\xF0\x42\x00\x00\x80\x3F\x00\x00\x80\x3F\x00\x00\x80\x3F\x00\x00\x80\x3F\x01\x00\x00\x00\x01\x00\x00\x00\x02\x00\x00\x00\x00\x00\x00\x00\x00\x00";
			int _written = filesystem->Write(dummy, sizeof(dummy)-1, f);
			filesystem->Flush(f);
			filesystem->Close(f);

#if TEST_STUFF
			filesystem->SetWarningLevel(FILESYSTEM_WARNING_QUIET);
			rootconsole->ConsolePrint("wrote %d bytes to dummy.nav", _written);
#endif

			auto original_mapname = *GlobalsMapname();
			*GlobalsMapname() = "dummy";
#if TEST_STUFF
			rootconsole->ConsolePrint("!!!!!!!!!!! overwrite mapname -- '%s'", gpGlobals->mapname);
#endif
			int ret = DETOUR_MEMBER_CALL(CNavMesh_Load)();
			*GlobalsMapname() = original_mapname;
			return ret;
		}
	}

#if TEST_STUFF
	filesystem->SetWarningLevel(FILESYSTEM_WARNING_QUIET);
#endif
	return DETOUR_MEMBER_CALL(CNavMesh_Load)();
}

bool Extension_OnLoad(char* error, size_t maxlength)
{
	if (!gameconfs->LoadGameConfigFile("srcwr.gamedata", &g_GameConfig, error, maxlength))
		return false;

	CDetourManager::Init(smutils->GetScriptingEngine(), g_GameConfig);

	g_CDownloadListGenerator_OnResourcePrecachedFullPath_detour = DETOUR_CREATE_MEMBER(
		CDownloadListGenerator_OnResourcePrecachedFullPath,
		"CDownloadListGenerator::OnResourcePrecachedFullPath"
	);

	g_CDownloadListGenerate_SetStringTable_detour = DETOUR_CREATE_MEMBER(
		CDownloadListGenerate_SetStringTable,
		"CDownloadListGenerate::SetStringTable"
	);

	g_CNavMesh_Load_detour = DETOUR_CREATE_MEMBER(
		CNavMesh_Load,
		"CNavMesh::Load"
	);

	g_CDownloadListGenerator_OnResourcePrecachedFullPath_detour->EnableDetour();
	g_CDownloadListGenerate_SetStringTable_detour->EnableDetour();
	g_CNavMesh_Load_detour->EnableDetour();

	sharesys->AddNatives(myself, UtilNatives);


	g_SmolStringListType = g_pHandleSys->CreateType(
		  "SmolStringList"
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
	g_CDownloadListGenerator_OnResourcePrecachedFullPath_detour->Destroy();
	g_CDownloadListGenerate_SetStringTable_detour->Destroy();
	g_CNavMesh_Load_detour->Destroy();
	gameconfs->CloseGameConfigFile(g_GameConfig);

	g_pHandleSys->RemoveType(g_SmolStringListType, myself->GetIdentity());
}

void Extension_OnAllLoaded() {}

static cell_t N_SRCWRUTIL_GetSHA1_File(IPluginContext* ctx, const cell_t* params)
{
	char* buffer;
	(void)ctx->LocalToString(params[1], &buffer);
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

	return rust_SRCWRUTIL_GetSHA1_File(fileobject, buffer);
}

// GMT... UTC+0... same thing...
static cell_t N_SRCWRUTIL_FormatTimeGMT(IPluginContext* ctx, const cell_t* params)
{
	// linux builds (because glibc 2.17?) have a 4-byte time_t???
	// TODO: -D_TIME_BITS=64 and -D_FILE_OFFSET_BITS=64 / etc ?
	// static_assert(sizeof(time_t) == 8);

	char *buffer, *format;
	(void)ctx->LocalToString(params[1], &buffer);
	int maxlength = params[2];
	(void)ctx->LocalToString(params[3], &format);
	time_t stamp = params[4]; // we have until 2038 to fuck this up...

	// if (stamp == -1) stamp = time(NULL);
	if (stamp == -1) stamp = g_pSM->GetAdjustedTime(); // TODO:

	struct tm gm;
#ifdef WIN32
	(void)_gmtime64_s(&gm, &stamp);
#else
	// glibc has had gmtime_r since 1995
	gmtime_r(&stamp, &gm);
#endif

	return strftime(buffer, maxlength, format, &gm);
}

extern const sp_nativeinfo_t UtilNatives[] = {
	{"SRCWRUTIL_GetSHA1_File", N_SRCWRUTIL_GetSHA1_File},
	{"SRCWRUTIL_FormatTimeGMT", N_SRCWRUTIL_FormatTimeGMT},
	{NULL, NULL}
};
