// SPDX-License-Identifier: GPL-3.0-or-later
// Copyright 2025 rtldg <rtldg@protonmail.com>
// This file is part of srcwrtimer (https://github.com/srcwr/srcwrtimer/)

#pragma newdecls required
#pragma semicolon 1

#include <srcwr/json> // needed before http...
#include <srcwr/http>

#undef REQUIRE_PLUGIN
#include <srcwr/core>

#include "srcwr/convar_class.inc"

public Plugin myinfo = {
	name = "[srcwr] geoip updater",
	author = "rtldg",
	description = "desc"...SRCWR_REPO_BASE_URL,
	version = SRCWR_VERSION,
	url = SRCWR_REPO_BASE_URL
};

char gS_mmdb[PLATFORM_MAX_PATH];
Handle gT_Timer = null;

public void OnPluginStart()
{
	BuildPath(Path_SM, gS_mmdb, sizeof(gS_mmdb), "configs/geoip/GeoLite2-City.mmdb");
	gT_Timer = CreateTimer(60.0 * 60.0 * 12, Timer_Update, 0, TIMER_REPEAT);
	TriggerTimer(gT_Timer, true);
	RegAdminCmd("sm_updategeoip", Command_Update, ADMFLAG_ROOT, "balls");
}

Action Command_Update(int client, int argc)
{
	TriggerTimer(gT_Timer, true);
	return Plugin_Handled;
}

void Timer_Update(Handle timer)
{
	int last_changed = GetFileTime(gS_mmdb, FileTime_LastChange);

	if (last_changed == -1 || (GetTime() - last_changed) > 60 * 60 * 24 * 14)
	{
		SRCWRHTTPReq req = new SRCWRHTTPReq("https://api.github.com/repos/P3TERX/GeoLite.mmdb/releases/latest");
		req.header("content-type", "application/json");
		req.YEET(ReleasesCallback);
	}
}

void ReleasesCallback(any value, const char[] error, SRCWRHTTPResp resp)
{
	if (error[0] != '\0')
	{
		LogError("Failed to fetch github release information. Error: %s", error);
		return;
	}

	SRCWRJSON json = resp.json;

	for (int i = 0, len = json.len(0, "/assets"); i < len; i++)
	{
		char filename[128];
		json.GetString(filename, sizeof(filename), 0, "/assets/%d/name", i);

		if (StrEqual(filename, "GeoLite2-City.mmdb"))
		{
			char url[400];
			json.ToFile("out.json");
			json.GetString(url, sizeof(url), 0, "/assets/%d/browser_download_url", i);

			SRCWRHTTPReq req = new SRCWRHTTPReq(url);
			req.Download(DownloadCallback, 0, "%s.tmp", gS_mmdb);
			break;
		}
	}

	delete json;
}

void DownloadCallback(any value, const char[] error, const char[] filename)
{
	if (error[0] != '\0')
	{
		LogError("Failed to download to %s. Error: %s", filename, error);
		return;
	}

	// TODO: RenameFile in a loop/timer
	if (RenameFile(gS_mmdb, filename))
	{
		LogMessage("Successfully updated geoip db!");
	}
	else
	{
		LogError("Failed to rename %s to %s", filename, gS_mmdb);
		DeleteFile(filename);
	}
}
