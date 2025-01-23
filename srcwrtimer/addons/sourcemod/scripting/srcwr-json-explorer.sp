// SPDX-License-Identifier: GPL-3.0-or-later
// Copyright 2025 rtldg <rtldg@protonmail.com>
// This file is part of srcwrtimer (https://github.com/srcwr/srcwrtimer/)

#pragma newdecls required
#pragma semicolon 1

#include <adminmenu>

#include <srcwr/core>


public Plugin myinfo = {
	name = "[srcwr] json explorer",
	author = "rtldg",
	description = "desc"...SRCWR_REPO_BASE_URL,
	version = SRCWR_VERSION,
	url = SRCWR_REPO_BASE_URL
};


int gI_Menu1Pos[MAXPLAYERS+1];
ArrayStack gA_Menu3Pos[MAXPLAYERS+1];
int gI_MenuTargetSerial[MAXPLAYERS+1];
char gS_MenuObjectType[MAXPLAYERS+1][9];
char gS_MenuKey[MAXPLAYERS+1][512];
SRCWRJSON gJ_MenuObject[MAXPLAYERS+1];


public void OnPluginStart()
{
	SRCWR_GetHandles();
	LoadTranslations("common.phrases"); // FindTarget() needs translations from this...
	RegAdminCmd("sm_json", Command_Json, ADMFLAG_ROOT, "sm_json <#userid|name> <t{imer}|p{player}|s{settings}> <key> [<type> <value>]");
}

Action Command_Json(int client, int argc)
{
	if (argc == 0 && client)
	{
		gJ_Timer[client].SetBool(true, 0, "fuck");
		gI_Menu1Pos[client] = 0;
		OpenMenu1(client);
		return Plugin_Handled;
	}

	if (argc != 3 && argc != 5)
	{
		ReplyToCommand(client, "Need 3 or 5 args. %d passed. Usage: sm_json <#userid|name> <t{imer}|p{player}|s{settings}> <key> [<type> <value>]", argc);
		return Plugin_Handled;
	}

	bool set_json = argc == 5;

	char targetstr[256], obj[16], key[256], type[16], value[256];
	GetCmdArg(1, targetstr, sizeof(targetstr));
	GetCmdArg(2, obj, sizeof(obj));
	GetCmdArg(3, key, sizeof(key));
	if (set_json) GetCmdArg(4, type, sizeof(type));
	if (set_json) GetCmdArg(5, value, sizeof(value));

	int target = StrEqual(targetstr, "0") ? 0 : FindTarget(client, targetstr, false, false);

	if (target == -1)
	{
		return Plugin_Handled;
	}

	LowercaseString(obj);

	SRCWRJSON j = obj[0] == 't' ? gJ_Timer[target] :
		(obj[0] == 'p' ? gJ_Player[target] : gJ_Settings[target]);

	if (!set_json)
	{
		j.ToString(value, sizeof(value), 0, key);
		ReplyToCommand(client, "%s", value);
		return Plugin_Handled;
	}

	LowercaseString(type);

	if (StrEqual(type, "f32") || StrEqual(type, "float"))
	{
		j.SetF32(StringToFloat(value), J_CREATE_PARENTS, key);
	}
	else if (StrEqual(type, "int") || StrEqual(type, "i32"))
	{
		j.SetCell(StringToInt(value), J_CREATE_PARENTS, key);
	}
	else if (StrEqual(type, "bool"))
	{
		bool b;
		if (StrEqual(value, "true"))
			b = true;
		else if (StrEqual(value, "false"))
			b = false;
		else
			b = StringToInt(value) != 0;
		j.SetBool(b, J_CREATE_PARENTS, key);
	}
	else if (StrEqual(type, "null"))
	{
		j.SetNull(J_CREATE_PARENTS, key);
	}
	else if (StrEqual(type, "str") || StrEqual(type, "s") || StrEqual(type, "string"))
	{
		j.SetString(value, 0, J_CREATE_PARENTS, key);
	}
	else
	{
		ReplyToCommand(client, "Unknown json type '%s'. Available: f32/float, int/i32, bool, null, s/str/string.", type);
	}

	return Plugin_Handled;
}

void OpenMenu1(int client)
{
	Menu menu = new Menu(MenuHandler_Menu1);
	menu.SetTitle("json explorer\n ");
	menu.ExitButton = true;
	menu.AddItem("0", "SERVER");
	AddTargetsToMenu2(menu, client, COMMAND_FILTER_CONNECTED);
	menu.DisplayAt(client, gI_Menu1Pos[client], MENU_TIME_FOREVER);
}

int MenuHandler_Menu1(Menu menu, MenuAction action, int param1, int param2)
{
	if (action == MenuAction_Select)
	{
		int client = param1;
		int item = param2;

		char info[16], name[33];
		menu.GetItem(item, info, sizeof(info), _, name, sizeof(name));

		int userid = StringToInt(info);
		int target = GetClientOfUserId(userid);

		if (userid != 0 && target == 0)
		{
			PrintToChat(client, "[SM] %t", "Player no longer available");
		}
		else
		{
			gI_Menu1Pos[client] = GetMenuSelectionPosition();
			gI_MenuTargetSerial[client] = target ? GetClientSerial(target) : 0;
			OpenMenu2(client);
		}
	}
	else if (action == MenuAction_End)
	{
		delete menu;
	}

	return 0;
}

void OpenMenu2(int client)
{
	int target = GetClientFromSerial(gI_MenuTargetSerial[client]);

	if (gI_MenuTargetSerial[client] && target == 0)
	{
		PrintToChat(client, "[SM] %t", "Player no longer available");
		return;
	}

	Menu menu = new Menu(MenuHandler_Menu2);
	menu.SetTitle("json explorer - %N\n ", target);
	menu.ExitBackButton = true;
	menu.AddItem("Timer", "Timer");
	menu.AddItem("Player", "Player");
	menu.AddItem("Settings", "Settings");
	menu.Display(client, MENU_TIME_FOREVER);
}

int MenuHandler_Menu2(Menu menu, MenuAction action, int param1, int param2)
{
	if (action == MenuAction_Select)
	{
		int client = param1;
		int item = param2;

		int target = GetClientFromSerial(gI_MenuTargetSerial[client]);

		if (gI_MenuTargetSerial[client] && target == 0)
		{
			PrintToChat(client, "[SM] %t", "Player no longer available");
			return 0;
		}

		char info[9];
		menu.GetItem(item, info, sizeof(info));

		if (StrEqual(info, "Timer"))
		{
			gJ_MenuObject[client] = gJ_Timer[target];
		}
		else if (StrEqual(info, "Player"))
		{
			gJ_MenuObject[client] = gJ_Player[target];
		}
		else if (StrEqual(info, "Settings"))
		{
			gJ_MenuObject[client] = gJ_Settings[target];
		}

		gS_MenuObjectType[client] = info;
		gS_MenuKey[client] = "";
		delete gA_Menu3Pos[client];
		gA_Menu3Pos[client] = new ArrayStack(1);
		OpenMenu3(client, 0);
	}
	else if (action == MenuAction_Cancel && param2 == MenuCancel_ExitBack)
	{
		OpenMenu1(param1);
	}
	else if (action == MenuAction_End)
	{
		delete menu;
	}

	return 0;
}

void OpenMenu3(int client, int pos)
{
	int target = GetClientFromSerial(gI_MenuTargetSerial[client]);

	if (gI_MenuTargetSerial[client] && target == 0)
	{
		PrintToChat(client, "[SM] %t", "Player no longer available");
		return;
	}

	Menu menu = new Menu(MenuHandler_Menu3);
	menu.SetTitle(
		 "json explorer - %N - %s\n%s%s\n "
		, target
		, gS_MenuObjectType[client]
		, gS_MenuKey[client][0] == '\0' ? "root" : "path="
		, gS_MenuKey[client][0] == '\0' ? "" : gS_MenuKey[client]
	);
	menu.ExitBackButton = true;

	//gJ_MenuObject[client].ToServer();
	//PrintToServer("'%s'", gS_MenuKey[client]);

	J_Type parent_type = gJ_MenuObject[client].GetType(0, gS_MenuKey[client][0] != '\0' ? gS_MenuKey[client] : NULL_STRING);

	if (parent_type == J_Map)
	{
		ArrayList keys = new ArrayList(ByteCountToCells(128));
		gJ_MenuObject[client].FillKeys(keys, 0, gS_MenuKey[client][0] != '\0' ? gS_MenuKey[client] : NULL_STRING);

		for (int i = 0, len = keys.Length; i < len; ++i)
		{
			char key[128], display[512];
			keys.GetString(i, key, sizeof(key));

			J_Type type = gJ_MenuObject[client].GetType(0, "%s/%s", gS_MenuKey[client], key);

			if (type != J_Map && type != J_Array)
			{
				gJ_MenuObject[client].ToString(display, sizeof(display), 0, "%s/%s", gS_MenuKey[client], key);
				Format(display, sizeof(display), "%s=%s", key, display);
			}
			else
			{
				FormatEx(display, sizeof(display), "%s", key);
				//FormatEx(display, sizeof(display), "%s (%s)", key, gS_J_Type[type]);
			}

			menu.AddItem(key, display, (type != J_Map && type != J_Array) ? ITEMDRAW_DISABLED : 0);
		}

		delete keys;
	}
	else if (parent_type == J_Array)
	{
		for (int i = 0, len = gJ_MenuObject[client].len(0, gS_MenuKey[client][0] != '\0' ? gS_MenuKey[client] : NULL_STRING); i < len; ++i)
		{
			char key[128], display[512];
			IntToString(i, key, sizeof(key));

			J_Type type = gJ_MenuObject[client].GetType(0, "%s/%s", gS_MenuKey[client], key);

			if (type != J_Map && type != J_Array)
			{
				gJ_MenuObject[client].ToString(display, sizeof(display), 0, "%s/%s", gS_MenuKey[client], key);
				Format(display, sizeof(display), "[%s]=%s", key, display);
			}
			else
			{
				FormatEx(display, sizeof(display), "[%s]", key);
			}

			menu.AddItem(key, display, (type != J_Map && type != J_Array) ? ITEMDRAW_DISABLED : 0);
		}
	}
	else
	{
		// :thinking:
	}

	if (menu.ItemCount == 0)
		menu.AddItem("", "empty :(", ITEMDRAW_DISABLED);

	menu.DisplayAt(client, pos, MENU_TIME_FOREVER);
}

int MenuHandler_Menu3(Menu menu, MenuAction action, int param1, int param2)
{
	if (action == MenuAction_Select)
	{
		int client = param1;
		int item = param2;

		int target = GetClientFromSerial(gI_MenuTargetSerial[client]);

		if (gI_MenuTargetSerial[client] && target == 0)
		{
			PrintToChat(client, "[SM] %t", "Player no longer available");
			return 0;
		}

		char info[128];
		menu.GetItem(item, info, sizeof(info));

		Format(gS_MenuKey[client], sizeof(gS_MenuKey[client]), "%s/%s", gS_MenuKey[client], info);
		gA_Menu3Pos[client].Push(GetMenuSelectionPosition());
		OpenMenu3(client, 0);
	}
	else if (action == MenuAction_Cancel && param2 == MenuCancel_ExitBack)
	{
		int client = param1;

		int idx = FindCharInString(gS_MenuKey[client], '/', true);

		if (idx == -1)
		{
			OpenMenu2(client);
		}
		else
		{
			gS_MenuKey[client][idx] = '\0';
			int pos = gA_Menu3Pos[client].Empty ? 0 : gA_Menu3Pos[client].Pop();
			OpenMenu3(client, pos);
		}
	}
	else if (action == MenuAction_End)
	{
		delete menu;
	}

	return 0;
}
