// SPDX-License-Identifier: GPL-3.0-or-later
// Copyright 2022 rtldg <rtldg@protonmail.com>
// This file is part of srcwrtimer (https://github.com/srcwr/srcwrtimer/)

#include <srcwr/syncer>

#define REPO_PATH "addons/sourcemod/data/testrepo"
#define REPO_URL "https://github.com/rtldg/mpbhops_but_working.git"

public void OnPluginStart()
{
	RegAdminCmd("sm_sclone", Command_Clone, ADMFLAG_RCON, "");
	RegAdminCmd("sm_spull", Command_Pull, ADMFLAG_RCON, "");
	DirectoryListing dir = OpenDirectory(REPO_PATH);
}

public Action Command_Clone(int client, int args)
{
	SRCWR_Syncer_CloneRepo(REPO_URL, REPO_PATH, Sync_Callback, 0);
	return Plugin_Handled;
}

public Action Command_Pull(int client, int args)
{
	SRCWR_Syncer_PullRepo("main", REPO_PATH, Sync_Callback, 0);
	return Plugin_Handled;
}

void Sync_Callback(any data, const char[] error)
{
	PrintToServer("data = %d | error = '%s'", data, error);
}
