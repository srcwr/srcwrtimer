// SPDX-License-Identifier: GPL-3.0-or-later
// Copyright 2025 rtldg <rtldg@protonmail.com>
// This file is part of srcwrtimer (https://github.com/srcwr/srcwrtimer/)

// This plugin is a clone of https://github.com/BoomShotKapow/GetMap, although no code was used or looked at.
// I stole the convars though.

#pragma newdecls required
#pragma semicolon 1

#include <bzip2>
#include <srcwr/http>

#undef REQUIRE_PLUGIN
#include <srcwr/core>

#include "srcwr/convar_class.inc"

public Plugin myinfo = {
	name = "[srcwr] getmap",
	author = "rtldg (& BoomShotKapow for the original)",
	description = "desc"...SRCWR_REPO_BASE_URL,
	version = SRCWR_VERSION,
	url = SRCWR_REPO_BASE_URL
};

Convar gCV_URL = null;
Convar gCV_FastdlPath = null;
Convar gCV_MapsPath = null;
Convar gCV_ReplaceMap = null;
//Convar gCV_MapPrefix = null;

public void OnPluginStart()
{
	RegAdminCmd("sm_getmap", Command_GetMap, ADMFLAG_ROOT, "Downloads (and extracts) maps from an http server. Usage: sm_getmap <mapname>");

	gCV_URL = new Convar("srcwr_getmap_url", "https://main.fastdl.me/maps/", "TODO: description", FCVAR_PROTECTED);
	gCV_FastdlPath = new Convar("srcwr_getmap_fastdl_path", "", "TODO: description. If empty, then will delete a .bz2 file if downloaded", FCVAR_PROTECTED);
	gCV_MapsPath = new Convar("srcwr_getmap_maps_path", "maps/", "TODO: description", FCVAR_PROTECTED);
	gCV_ReplaceMap = new Convar("srcwr_getmap_replace_map", "1", "TODO: description", 0, true, 0.0, true, 1.0);
	Convar.AutoExecConfig();
}

Action Command_GetMap(int client, int argc)
{
	if (argc == 0)
	{
		ReplyToCommand(client, "Usage: sm_getmap <mapname>");
		return Plugin_Handled;
	}

	char url[256], mapspath[PLATFORM_MAX_PATH], fastdlpath[PLATFORM_MAX_PATH];
	gCV_URL.GetString(url, sizeof(url));
	gCV_MapsPath.GetString(mapspath, sizeof(mapspath));
	gCV_FastdlPath.GetString(fastdlpath, sizeof(fastdlpath));

	if (url[0] == '\0')
	{
		ReplyToCommand(client, "URL is empty for getmap!");
		return Plugin_Handled;
	}

	if (mapspath[0] == '\0')
	{
		ReplyToCommand(client, "Maps path not set for getmap!");
		return Plugin_Handled;
	}

	char mapname[MAPNAMEBUFSZ];
	GetCmdArgString(mapname, sizeof(mapname));
	TrimString(mapname);

	if (mapname[0] == '\0')
	{
		ReplyToCommand(client, "Empty mapname for getmap!");
		return Plugin_Handled;
	}

	if (!gCV_ReplaceMap.BoolValue)
	{
		// check if exists in fastdl or maps folder
	}

	ReplyToCommand(client, "Attempting to download %s", mapname);

	DataPack dp = new DataPack();
	dp.WriteCell(client ? GetClientSerial(client) : 0);
	dp.WriteString(mapname);
	dp.WriteCell(0); // 0 = try .bsp.bz2, 1 = try .bsp, 2 = tried .bsp...
	TryDownload(dp);

	return Plugin_Handled;
}

void TryDownload(DataPack dp)
{
	dp.Reset();
	int client = GetClientFromSerial(dp.ReadCell());

	char url[256], mapname[MAPNAMEBUFSZ], path[PLATFORM_MAX_PATH];
	gCV_URL.GetString(url, sizeof(url));

	dp.ReadString(mapname, sizeof(mapname));
	DataPackPos pos = dp.Position; // save this so we can overwrite it...
	int state = dp.ReadCell();
	dp.Position = pos; // reset pos...
	dp.WriteCell(state + 1, true); // overwrite that sucker...

	SRCWRHTTPReq req = new SRCWRHTTPReq(state ? "%s%s.bsp" : "%s%s.bsp.bz2", url, mapname);

	if (state == 0)
		gCV_FastdlPath.GetString(path, sizeof(path));

	if (path[0] == '\0')
		gCV_MapsPath.GetString(path, sizeof(path));

	if (path[strlen(path)-1] != '/')
		StrCat(path, sizeof(path), "/");

	req.Download(DownloadCallback, dp, state ? "%s%s.bsp" : "%s%s.bsp.bz2", path, mapname);
}

void DownloadCallback(any data, const char[] error, const char[] filename)
{
	DataPack dp = data;
	dp.Reset();
	int client = GetClientFromSerial(dp.ReadCell());
	char mapname[MAPNAMEBUFSZ];
	dp.ReadString(mapname, sizeof(mapname));
	int state = dp.ReadCell();

	if (error[0] != '\0')
	{
		if (state == 1)
		{
			TryDownload(dp);
			return;
		}

		ReplyToCommand(client, "Failed to download %s! Error: %s", mapname, error);

		delete dp;
		return;
	}

	if (state == 1)
	{
		// I FUCKING LOVE FETCHING STRINGS ALL THE TIME HAHAHAHAHAHAHA :|

		char bsppath[PLATFORM_MAX_PATH];
		gCV_MapsPath.GetString(bsppath, sizeof(bsppath));
		if (bsppath[strlen(bsppath)-1] != '/')
			StrCat(bsppath, sizeof(bsppath), "/");
		Format(bsppath, sizeof(bsppath), "%s%s.bsp", bsppath, mapname);

		BZ2_DecompressFile(filename, bsppath, DecompressCallback, dp);
	}
	else
	{
		char fastdlpath[PLATFORM_MAX_PATH];
		gCV_FastdlPath.GetString(fastdlpath, sizeof(fastdlpath));

		if (fastdlpath[0] == '\0')
		{
			delete dp;
			YayToClient(client, mapname);
		}
		else
		{
			if (fastdlpath[strlen(fastdlpath)-1] != '/')
				StrCat(fastdlpath, sizeof(fastdlpath), "/");
			Format(fastdlpath, sizeof(fastdlpath), "%s%s.bsp.bz2", fastdlpath, mapname);
			BZ2_CompressFile(filename, fastdlpath, 9, CompressCallback, dp);
		}
	}
}

void DecompressCallback(BZ_Error error, char[] infile, char[] outfile, any data)
{
	DataPack dp = data;
	dp.Reset();
	int client = GetClientFromSerial(dp.ReadCell());
	char mapname[MAPNAMEBUFSZ];
	dp.ReadString(mapname, sizeof(mapname));
	delete dp;

	if (error == BZ_OK)
	{
		YayToClient(client, mapname);

		char fastdlpath[PLATFORM_MAX_PATH];
		gCV_FastdlPath.GetString(fastdlpath, sizeof(fastdlpath));

		//PrintToServer("'%s' '%s' '%s'", fastdlpath, infile, outfile);

		if (fastdlpath[0] == '\0')
		{
			DeleteFile(infile);
		}
	}
	else
	{
		DeleteFile(infile);
		DeleteFile(outfile);

		char estr[255];
		BZ2_Error(error, estr, sizeof(estr));
		ReplyToCommand(client, "Failed to extract %s.bsp.bz2! (%s)", mapname, estr);
		LogError("Failed to extract %s.bsp.bz2! (%s)", mapname, estr);
	}
}

void CompressCallback(BZ_Error error, char[] infile, char[] outfile, any data)
{
	DataPack dp = data;
	dp.Reset();
	int client = GetClientFromSerial(dp.ReadCell());
	char mapname[MAPNAMEBUFSZ];
	dp.ReadString(mapname, sizeof(mapname));
	delete dp;

	if (error == BZ_OK)
	{
		YayToClient(client, mapname);
	}
	else
	{
		DeleteFile(outfile);

		char estr[255];
		BZ2_Error(error, estr, sizeof(estr));
		ReplyToCommand(client, "Failed to compress %s.bsp to a .bz2! (%s)", mapname, estr);
		LogError("Failed to compress %s.bsp to a .bz2! (%s)", mapname, estr);
	}
}

void YayToClient(int client, const char[] mapname)
{
	ReplyToCommand(client, "Downloaded %s", mapname);
}
