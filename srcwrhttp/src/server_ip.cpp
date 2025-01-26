// SPDX-License-Identifier: WTFPL
// Copyright 2022-2025 rtldg <rtldg@protonmail.com>
// This file is part of srcwrtimer (https://github.com/srcwr/srcwrtimer/)

#include <sm_platform.h> // PLATFORM_EXTERN_C

#define USELIBSYS 1
#if USELIBSYS
#include <ILibrarySys.h>
extern SourceMod::ILibrarySys *libsys;
#endif

struct steamip
{
	union {
		unsigned ipv4;
		unsigned char ipv6[16];
	};
	unsigned type;
};

typedef void* (__cdecl *pSteamAPI_SteamGameServer_v01x)();
typedef struct steamip (__cdecl *pSteamAPI_ISteamGameServer_GetPublicIP)(void*);

static pSteamAPI_SteamGameServer_v01x getGameServer;
static pSteamAPI_ISteamGameServer_GetPublicIP getPublicIP;
static void* gameServer;

PLATFORM_EXTERN_C unsigned SteamWorks_GetPublicIP()
{
	if (!gameServer)
	{
		auto lib = libsys->OpenLibrary(
	#ifdef _WIN32
			"./bin/steam_api.dll",
	#else
			"./bin/libsteam_api.so",
	#endif
			NULL, 0);
		if (!lib) return 0;
		getGameServer = (pSteamAPI_SteamGameServer_v01x)lib->GetSymbolAddress("SteamAPI_SteamGameServer_v013"); // cstrike/tf
		if (!getGameServer) return 0;
		getPublicIP = (pSteamAPI_ISteamGameServer_GetPublicIP)lib->GetSymbolAddress("SteamAPI_ISteamGameServer_GetPublicIP");
		gameServer = (*getGameServer)();
	}
	if (!getPublicIP || !gameServer) return 0;
	return (*getPublicIP)(gameServer).ipv4;
}
