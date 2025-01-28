// SPDX-License-Identifier: GPL-3.0-or-later
// Copyright 2022-2025 rtldg <rtldg@protonmail.com>
// This file is part of srcwrtimer (https://github.com/srcwr/srcwrtimer/)

#if defined _srcwr_core_included
	#endinput
#endif
#define _srcwr_core_included
#pragma semicolon 1

#include "json.inc"

#if !defined SRCWR_VERSION
#define SRCWR_VERSION_STR "1.0.0"
#define SRCWR_MAJOR 1
#define SRCWR_MINOR 0
#define SRCWR_PATCH 0
#if defined SRCWR_VERSION_GIT
#define SRCWR_VERSION SRCWR_VERSION_STR ... "-" ... SRCWR_VERSION_GIT
#else
#define SRCWR_VERSION SRCWR_VERSION_STR
#endif
#endif

#if !defined SRCWR_REPO_BASE_URL
#define SRCWR_REPO_BASE_URL "https://github.com/srcwr/srcwrtimer"
#endif
#define SP_URL(%1) SRCWR_REPO_BASE_URL..."/blob/"...SRCWR_VERSION..."/srcwrtimer/addons/sourcemod/scripting/"...%1...".sp"

#define MAPNAMEBUFSZ 129 // TODO: Engine uses char[128] for the mapname stem?
#define CSS_MAXPLAYERS 65 // thanks tf2 for adding 100 player support for some reason
#define MAX_NETWORKED_ENTS 2048

//////////////////////////////////////////////////////////////////////////
// macros
//////////////////////////////////////////////////////////////////////////

// stolen from boosterfix
#define EXPAND_VECTOR(%1) %1[0], %1[1], %1[2]

stock int imin(int a, int b) { return a < b ? a : b; }
stock int imax(int a, int b) { return a > b ? a : b; }
stock float fmin(float a, float b) { return a < b ? a : b; }
stock float fmax(float a, float b) { return a > b ? a : b; }
stock int iclamp(int val, int min, int max) { if (val < min) return min; if (val > max) return max; return val; }
stock float fclamp(float val, float min, float max) { if (val < min) return min; if (val > max) return max; return val; }

#define BITFLAGARRAY(%1,%2) int %1[((%2) + 31 / 32)]
#define BITFLAGARRAY_HAS(%1, %2) ((%1[(%2)/32] & 1 << ((%2) % 32)) != 0)
#define BITFLAGARRAY_SET(%1, %2) (%1[(%2)/32] |= 1 << ((%2) % 32))
#define BITFLAGARRAY_CLR(%1, %2) (%1[(%2)/32] &= ~(1 << ((%2) % 32)))

//////////////////////////////////////////////////////////////////////////
// global variables
//////////////////////////////////////////////////////////////////////////

// The current run's state. Akin to bhoptimer's `timer_snapshot_t`.
SRCWRJSON gJ_Timer[MAXPLAYERS+1];
// The main state of players/clients in a server.
// Contains:
// - array of checkpoints (which are copies of gJ_Timer[] objects...)
// - playtime tracker
// - cached rank info
// - other shit... idk
// [0] is special and for the server. If you clobber it then you'll break everything.
SRCWRJSON gJ_Player[MAXPLAYERS+1];
// Settings and shit for clients...
// Do not manually SET values because the `SRCWR_OnEditPlayerSettings()` forward will not trigger for plugins.
SRCWRJSON gJ_Settings[MAXPLAYERS+1];

//////////////////////////////////////////////////////////////////////////
// enums and structs
//////////////////////////////////////////////////////////////////////////

// stolen from bhoptimer
enum
{
	Track_Main,
	Track_Bonus,
	Track_Bonus_Last = 9,
	TRACKS_SIZE
}

// stolen from bhoptimer
enum TimerStatus
{
	Timer_Stopped,
	Timer_Running,
	Timer_Paused
}

enum
{
	SRCWR_Windows = 1,
	SRCWR_Linux = 2,
}

// TODO: Remove
// stolen from bhoptimer
enum struct chatstrings_t
{
	char sPrefix[64];
	char sText[16];
	char sWarning[16];
	char sVariable[16];
	char sVariable2[16];
	char sStyle[16];
}
chatstrings_t gS_ChatStrings;

methodmap SRCWRDatabase < Handle {
}

//////////////////////////////////////////////////////////////////////////
// stocks - settings related
//////////////////////////////////////////////////////////////////////////

// You can consider the maximum path of a setting to be char[128]. That's already stupidly long...
stock void SRCWR_SetDefaultPlayerSettings(const char[] settings_name, const char[] settings_str)
{
	if (!gJ_Settings[0].SetFromString(settings_str, 0, J_CREATE_PARENTS, "/player_defaults/%s", settings_name))
		SetFailState("Failed to set default player settings (/%s). Check for trailing commas.\n\n%s\n", settings_name, settings_str);
}

//////////////////////////////////////////////////////////////////////////
// stocks - SRCWR related
//////////////////////////////////////////////////////////////////////////

stock bool SRCWR_GetHandles(char[] error="", int err_max=0)
{
	if (   !SRCWRJSON.GetHandles("timer_state", gJ_Timer, sizeof(gJ_Timer))
	    || !SRCWRJSON.GetHandles("player_state", gJ_Player, sizeof(gJ_Player))
	    || !SRCWRJSON.GetHandles("player_settings", gJ_Settings, sizeof(gJ_Settings))
	)
	{
		strcopy(error, err_max, "failed to create SRCWR JSON handles...");
		return false;
	}

	return true;
}

// TODO: swap key & delay?
stock bool SRCWR_Ratelimit_Realtime(int client, const char[] key, float delay)
{
	float now = GetEngineTime();
	float thing = gJ_Player[client].GetCell(0, key);
	if (thing > now)
		return false;
	gJ_Player[client].SetCell(now + delay, 0, key);
	return true;
}

stock void SRCWR_SetPracticeMode(int client, bool alert)
{
	if (gJ_Timer[client].SetBool(true, J_IS_DIFFERENT, "practice"))
	{
		if (alert && gJ_Settings[client].GetBool(0, "/hud/practice_alert"))
		{
			SRCWR_PrintToChat(client, "%T", "PracticeModeAlert", client, gS_ChatStrings.sWarning, gS_ChatStrings.sText);
		}
	}
}

stock TimerStatus SRCWR_GetTimerStatus(int client)
{
	if (!gJ_Timer[client].GetBool(0, "enabled"))
		return Timer_Stopped;
	if (gJ_Timer[client].GetBool(0, "paused"))
		return Timer_Paused;
	return Timer_Running;
}

stock void SRCWR_PrintSteamIDOnce(int client, int steamid, const char[] playername)
{
	if (gJ_Player[client].SetCell(steamid, J_IS_DIFFERENT, "last_printed_steamid"))
	{
		char steam2[20], steam64[20];
		FormatEx(steam2, sizeof(steam2), "STEAM_0:%d:%d", steamid&1, (steamid>>1) & 0x7FFFFFFF);
		int num[2] = {0, 0x01100001}; num[0] = steamid;
		Int64ToString(num, steam64, sizeof(steam64));
		SRCWR_PrintToChat(client, "%s: %s%s %s[U:1:%u]%s %s", playername, gS_ChatStrings.sVariable, steam2, gS_ChatStrings.sText, steamid, gS_ChatStrings.sVariable, steam64);
	}
}

stock bool SRCWR_ShouldProcessFrame(int client)
{
	// TODO: GetF32 and stop casting `any` to `float`?
	return gJ_Timer[client].GetCell(0, "timescale") == 1.0
	    || gJ_Timer[client].GetCell(0, "next_frame_time") <= 0.0;
}

// TODO:
//stock void GetTrackName()
//stock bool GuessBestMapName() // TODO: Use helper extension???
//stock void PrettyishTimescale() // ???? Not sure...
//stock void FormatSeconds() /// what a fucking mess this func is... allow to be overridden by custom or something... split?

// TODO:
// DB FUNCTIONS... ASYNC should use new extensions... SYNC = regular database handles....???
// Add LogQueryFunction....

//////////////////////////////////////////////////////////////////////////
// stocks - utilities
//////////////////////////////////////////////////////////////////////////

stock void LowercaseString(char[] str)
{
	for (int i, x; (x = str[i]) != 0; ++i)
	{
		if ('A' <= x <= 'Z')
			str[i] += ('a' - 'A');
	}
}

stock void UppercaseString(char[] str)
{
	for (int i, x; (x = str[i]) != 0; ++i)
	{
		if ('a' <= x <= 'z')
			str[i] -= ('a' - 'A');
	}
}

// TODO: SRCWR_GetSpecTarget?
stock int GetSpectatorTarget(int client, int fallback = -1)
{
	return fallback;
}

//GetLowercaseMapName() // helper extension...
//LessStupidGetMapDisplayName() /// use helper extension

#if 0
stock float NormalizeAngle(float original_angle)
{
	float angle = original_angle % 360.0;
	angle = (angle + 360.0) % 360.0;
	if (angle > 180.0) angle -= 360.0;
	return angle;
}
#endif
stock float NormalizeAngleNoNaN(float angle)
{
	angle = angle - (RoundToFloor((angle + 180.0)/360.0))*360.0;
	if (angle != angle) angle = 0.0; // try not to propogate NaN!
	return angle;
}

//////////////////////////////////////////////////////////////////////////
// natives - misc
//////////////////////////////////////////////////////////////////////////

// If you add custom settings from a third-party plugin then srcwr.smx will need to load the translations.
// Otherwise shit won't display translated strings for keys.
native void SRCWR_LoadTranslations(const char[] file);
// If you add custom settings from a third-party plugin then you'll need to call this.
native void SRCWR_RecreateSettingsMenu();

//////////////////////////////////////////////////////////////////////////
// natives
//////////////////////////////////////////////////////////////////////////

native SRCWRDatabase SRCWR_GetDatabase(int& outdriver=0, char[] prefix=NULL_STRING); // TODO

native bool SRCWR_ChangeClientStyle(...); // TODO

native void SRCWR_Start(int client, int track); // ???
native bool SRCWR_Restart(int client, int track, bool force);
native void SRCWR_Finish(int client);
native bool SRCWR_PauseTimer(int client, bool force);
native bool SRCWR_ResumeTimer(int client /*, bool teleport=false*/);
native bool SRCWR_StopTimer(int client, bool force);

// TODO:
//native bool SRCWR_SetStyleSetting...(...) /// NAH... static style definitions... but also json object anyway...

// TODO:
//native int SRCWR_GetStyleCount(); // nah... probably...
//native void SRCWR_OrderedStyles(...) // nah... probably...
//native int SRCWR_GetStyleStringsStruct(...) // nah... json GetStruct... probably...
//native int SRCWR_GetStyleStrings(...) // nah... json GetString

// TODO:
//native int SRCWR_GetChatStrings(...) // nah... json GetString
//native int SRCWR_GetChatStringsStruct(...) // nah... json GetStruct

//TODO: Allow??? Blacklist styles maybe? I don't like VIP styles though...
//native bool SRCWR_HasStyleAccess(int client, int style);

//native int SRCWR_CanPause(int client);

native void SRCWR_StopChatSound();
native int SRCWR_PrintToChat(int client, const char[] format, any ...);
native void SRCWR_PrintToChatAll(const char[] format, any ...);

// TODO:
//native void SRCWR_LogMessage(const char[][ format, any ...);

// TODO: Copy from Shavit_UpdateLaggedMovement
native void SRCWR_UpdateLaggedMovement(int client, bool user_timescale=true);

//////////////////////////////////////////////////////////////////////////
// forwards - clients
//////////////////////////////////////////////////////////////////////////

// ????
forward void SRCWR_OnClientAbleToPlay(int client);

forward void SRCWR_OnClientOSDetected(int client, int os);
forward void SRCWR_OnClientEarlyAuthID(int client, int authid); // TODO: Rename to steamid?

forward void SRCWR_OnStyleChanged(int client, int oldstyle, int newstyle /*, bool manual*/);
forward void SRCWR_OnTrackChanged(int client, int oldtrack, int newtrack);

forward Action SRCWR_OnStartPre(int client, int track);
forward void SRCWR_OnStart(int client, int track);

forward Action SRCWR_OnFinishPre(int client);
forward void SRCWR_OnFinish(int client);

forward Action SRCWR_OnRestartPre(int client, int track);
forward void SRCWR_OnRestart(int client, int track);

forward Action SRCWR_OnEndPre(int client, int track);
forward void SRCWR_OnEnd(int client, int track);

forward void SRCWR_OnPause(int client); // TODO PRE?
forward void SRCWR_OnResume(int client); // TODO PRE?
forward Action SRCWR_OnStopPre(int client);
forward void SRCWR_OnStop(int client);

// TODO:
//forward void SRCWR_OnTimerIncrement(...)

//forward Action SRCWR_OnStyleCommandPre(...) // TODO:???

// TODO:
// We have a bunch of these OnPlayerRunCmd forwards because different plugins need to inject functionality in different places.
//
//
// Use `OnPlayerRunCmdPre()` (THE SOURCEMOD ONE! NOT SRCWR ONE!) for
// Use `OnPlayerRunCmdPost()` (THE SOURCEMOD ONE! NOT SRCWR ONE!) for

// Use this for
forward Action SRCWR_OnPlayerRunCmdPre(int client, int& buttons, int& impulse, float vel[3], float angles[3], int& weapon, int& subtype, int& cmdnum, int& tickcount, int& seed, int mouse[2]);

//forward Action SRCWR_OnPlayerRunCmd(int client, int& buttons, int& impulse, float vel[3], float angles[3], int& weapon, int& subtype, int& cmdnum, int& tickcount, int& seed, int mouse[2]);

// Called when SRCWR is finished with all of its logic inside `OnPlayerRunCmd()` but before it returns.
forward Action SRCWR_OnPlayerRunCmdPost(int client, int& buttons, int& impulse, float vel[3], float angles[3], int& weapon, int& subtype, int& cmdnum, int& tickcount, int& seed, int mouse[2]);



forward void SRCWR_OnProcessMovementPre(int client);
forward void SRCWR_OnProcessMovementPost(int client);

// TODO:
//forward void SRCWR_OnDynamicTimescaleChanged(int client, float oldtimescale, float newtimescale);
//forward void SRCWR_OnTimeOffsetCalculated(int client, int zonetype, float offset, float distance);

//////////////////////////////////////////////////////////////////////////
// forwards - plugin stuff
//////////////////////////////////////////////////////////////////////////

// When someone toggles a setting with the `!settings` menu or with `!settings /core/auto !toggle`
// When `is_being_toggled` then `value` is empty!!!! Vice versa!!!!
// When `result == Plugin_Continue` and `type == J_Bool`
// TODO: Documentation...
// Editing the `value` string buffer does not do anything.
forward Action SRCWR_OnEditPlayerSettings(int client, const char[] key, bool is_being_toggled, char[] value, J_Type type);

// Client is guaranteed to be ingame and authorized by the time this forward is called.
forward void SRCWR_OnSettingsLoaded(int client);

// TODO:
//forward void SRCWR_OnChatConfigLoaded();
//forward void SRCWR_OnDatabaseLoaded();
//forward void SRCWR_OnStyleConfigLoaded(int styles);

//////////////////////////////////////////////////////////////////////////
// plugin stuff...
//////////////////////////////////////////////////////////////////////////

public SharedPlugin __pl_srcwr_core = {
	name = "srcwr",
	file = "srcwr.smx",
#if defined REQUIRE_PLUGIN
	required = 1
#else
	required = 0
#endif
};

#if !defined REQUIRE_PLUGIN
public void __pl_srcwr_core_SetNTVOptional()
{
	// TODO:
}
#endif
