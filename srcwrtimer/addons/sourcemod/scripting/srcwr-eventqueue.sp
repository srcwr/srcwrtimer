// SPDX-License-Identifier: GPL-3.0-only
// Copyright 2021-2022 carnifex/hermansimensen <herman.simensen@gmail.com>
// Copyright 2021-2022 rtldg <rtldg@protonmail.com>
// Copyright 2022 GAMMACASE <darknesss456@mail.ru>
// This file is part of srcwrtimer (https://github.com/srcwr/srcwrtimer/)
//   Originally from eventqueuefix (https://github.com/hermansimensen/eventqueue-fix)
//   and edited to integrate with SRCWR

#pragma semicolon 1

#define DO_CMD_POST 1

#include <sdktools>
#include <sdkhooks>
#include <dhooks>
#include <srcwr/core>

public Plugin myinfo = {
	name = "[srcwr] eventqueue",
	author = "carnifex, rtldg, GAMMACASE",
	description = "Fork of eventqueuefix to integrate with SRCWR.",
	version = SRCWR_VERSION ... "-eqfix-1.3.1", // srcwr & eventqueuefix version...
	url = SP_URL("srcwr-eventqueue")
};

//////////////////////////////////////////////////////////////////////////
//
//////////////////////////////////////////////////////////////////////////

enum struct event_t
{
	char target[64];
	char targetInput[64];
	char variantValue[256];
	float delay;
	int activator;
	int caller;
	int outputID;
}
#define event_t_fmt "target,char,64|targetInput,char,64|variantValue,char,256|delay,float|activator,int|caller,int|outputID,int"

#define FLT_EPSILON 1.192092896e-07
// How many bits to use to encode an edict.
#define    MAX_EDICT_BITS                11            // # of bits needed to represent max edicts
// Max # of edicts in a level
#define    MAX_EDICTS                    (1<<MAX_EDICT_BITS)

// Used for networking ehandles.
#define NUM_ENT_ENTRY_BITS        (MAX_EDICT_BITS + 1)
#define NUM_ENT_ENTRIES            (1 << NUM_ENT_ENTRY_BITS)
#define ENT_ENTRY_MASK            (NUM_ENT_ENTRIES - 1)
#define INVALID_EHANDLE_INDEX    0xFFFFFFFF

//////////////////////////////////////////////////////////////////////////
// global variables
//////////////////////////////////////////////////////////////////////////

bool gB_Late = false;

Handle g_hFindEntityByName;
int g_iRefOffset;

//////////////////////////////////////////////////////////////////////////
// forwards - plugin
//////////////////////////////////////////////////////////////////////////

public APLRes AskPluginLoad2(Handle myself, bool late, char[] error, int err_max)
{
	gB_Late = late;
	RegPluginLibrary("srcwr-eventqueue");
	return APLRes_Success;
}

public void OnPluginStart()
{
	DoGameData();
	SRCWR_GetHandles();
	SRCWR_SetDefaultPlayerSettings("eventqueue", "{\"debug\": false}");

	if (gB_Late)
	{
		int entity = -1;
		while (-1 != (entity = FindEntityByClassname(entity, "func_button")))
		{
			SDKHook(entity, SDKHook_OnTakeDamage, Hook_Button_OnTakeDamage);
		}

		for (int i = 1; i <= MaxClients; ++i)
		{
			if (IsClientInGame(i) && !gJ_Timer[i].Has(0, "eventqueue"))
			{
				OnClientPutInServer(i);
			}
		}
	}
}

//////////////////////////////////////////////////////////////////////////
// forwards - misc
//////////////////////////////////////////////////////////////////////////

public void OnEntityCreated(int entity, const char[] classname)
{
	if (StrEqual(classname, "func_button"))
	{
		SDKHook(entity, SDKHook_OnTakeDamage, Hook_Button_OnTakeDamage);
	}
}

public Action Hook_Button_OnTakeDamage(int victim, int& attacker, int& inflictor, float& damage, int& damagetype, int& weapon, float damageForce[3], float damagePosition[3], int damagecustom)
{
	if (1 <= attacker <= MaxClients && gJ_Settings[attacker].GetBool(0, "/eventqueue/debug"))
	{
		int prev = GetEntPropEnt(victim, Prop_Data, "m_hActivator");
		if (prev != attacker)
		{
			PrintToChat(attacker, "eqfix: Fixed button dmg activator");
		}
	}
	// func_button fires the OnDamage output before setting m_hActivator to the attacker.
	// This means m_hActivator can be unset or a previous attacker.
	// This is a problem at bhop_badges level 13 and also the booster in the ladder room.
	SetEntPropEnt(victim, Prop_Data, "m_hActivator", attacker);
	return Plugin_Continue;
}

//////////////////////////////////////////////////////////////////////////
// forwards - clients
//////////////////////////////////////////////////////////////////////////

public void OnClientPutInServer(int client)
{
	if (IsFakeClient(client))
		return;

	gJ_Timer[client].SetFromString(
		"{                          \
			\"timescale\":   1.0,   \
			\"outputwaits\": [],    \
			\"events\":      []     \
		}",
		-1, 0, "eventqueue"
	);
	SRCWR_LoadDefaultPlayerSettings(client, "eventqueue");
}

public void OnClientDisconnect_Post(int client)
{
	// TODO: remove this since srcwr-core will just clear the entire object....
	//gJ_Timer[client].Remove("eventqueuefix");
}

//////////////////////////////////////////////////////////////////////////
//
//////////////////////////////////////////////////////////////////////////

public void DoGameData()
{
	GameData gamedata = new GameData("srcwr.gamedata");

	int m_RefEHandleOff = gamedata.GetOffset("m_RefEHandle");
	int ibuff = gamedata.GetOffset("m_angRotation");
	g_iRefOffset = ibuff + m_RefEHandleOff;

	if (gamedata.GetOffset("FindEntityByName_StaticCall") == 1)
		StartPrepSDKCall(SDKCall_Static);
	else
		StartPrepSDKCall(SDKCall_EntityList);

	if (!PrepSDKCall_SetFromConf(gamedata, SDKConf_Signature, "FindEntityByName"))
		SetFailState("Failed to find FindEntityByName signature.");

	PrepSDKCall_SetReturnInfo(SDKType_PlainOldData, SDKPass_ByValue);
	PrepSDKCall_AddParameter(SDKType_CBaseEntity, SDKPass_Pointer, VDECODE_FLAG_ALLOWNULL | VDECODE_FLAG_ALLOWWORLD);
	PrepSDKCall_AddParameter(SDKType_String, SDKPass_Pointer);
	PrepSDKCall_AddParameter(SDKType_CBaseEntity, SDKPass_Pointer, VDECODE_FLAG_ALLOWNULL | VDECODE_FLAG_ALLOWWORLD);
	PrepSDKCall_AddParameter(SDKType_CBaseEntity, SDKPass_Pointer, VDECODE_FLAG_ALLOWNULL | VDECODE_FLAG_ALLOWWORLD);
	PrepSDKCall_AddParameter(SDKType_CBaseEntity, SDKPass_Pointer, VDECODE_FLAG_ALLOWNULL | VDECODE_FLAG_ALLOWWORLD);
	PrepSDKCall_AddParameter(SDKType_PlainOldData, SDKPass_ByValue);
	g_hFindEntityByName = EndPrepSDKCall();

	Handle addEventThree = DHookCreateDetour(Address_Null, CallConv_THISCALL, ReturnType_Void, ThisPointer_Ignore);

	if (!DHookSetFromConf(addEventThree, gamedata, SDKConf_Signature, "AddEventThree"))
		SetFailState("Failed to find AddEventThree signature.");

	DHookAddParam(addEventThree, HookParamType_CharPtr);
	DHookAddParam(addEventThree, HookParamType_CharPtr);
	if (gamedata.GetOffset("server_os") == SRCWR_Linux)
		DHookAddParam(addEventThree, HookParamType_ObjectPtr);
	else
		DHookAddParam(addEventThree, HookParamType_Object, 20);
	DHookAddParam(addEventThree, HookParamType_Float);
	DHookAddParam(addEventThree, HookParamType_Int);
	DHookAddParam(addEventThree, HookParamType_Int);
	DHookAddParam(addEventThree, HookParamType_Int);

	if (!DHookEnableDetour(addEventThree, false, DHook_AddEventThree))
		SetFailState("Failed to find AddEventThree signature.");

	Handle activateMultiTrigger = DHookCreateDetour(Address_Null, CallConv_THISCALL, ReturnType_Void, ThisPointer_CBaseEntity);

	if (!DHookSetFromConf(activateMultiTrigger, gamedata, SDKConf_Signature, "ActivateMultiTrigger"))
		SetFailState("Failed to find ActivateMultiTrigger signature.");

	DHookAddParam(activateMultiTrigger, HookParamType_CBaseEntity);

	if (!DHookEnableDetour(activateMultiTrigger, false, DHook_ActivateMultiTrigger))
		SetFailState("Couldn't enable ActivateMultiTrigger detour.");

	delete gamedata;
}

void Runner(int client)
{
	if (gJ_Timer[client].GetBool(0, "paused") || !SRCWR_ShouldProcessFrame(client))
		return;

	float timescale = gJ_Timer[client].GetF32(0, "/eventqueue/timescale");

	int count = gJ_Timer[client].len(0, "/eventqueue/outputwaits");

	for (int i = 0; i < count; ++i)
	{
		float waitTime = gJ_Timer[client].GetF32(
			0,
			"/eventqueue/outputwaits/%d/waitTime",
			i
		);

		if ((waitTime -= timescale) <= timescale)
		{
			gJ_Timer[client].Remove(0, "/eventqueuefix/outputwaits/%d", i);
			i -= 1;
			count -= 1;
		}
		else
		{
			gJ_Timer[client].SetF32(
				waitTime,
				0,
				"/eventqueuefix/outputwaits/%d/waitTime",
				i
			);
		}
	}

	count = gJ_Timer[client].len(0, "/eventqueue/events");
	bool dbgprint = gJ_Settings[client].GetBool(0, "/eventqueue/debug");

	for (int i = 0; i < count; ++i)
	{
		float delay = gJ_Timer[client].GetF32(
			0,
			"/eventqueue/events/%d/delay",
			i
		);

		if ((delay -= timescale) <= -timescale)
		{
			event_t event;
			gJ_Timer[client].GetStruct(
				event,
				event_t_fmt,
				0,
				"/eventqueue/events/%d",
				i
			);
			gJ_Timer[client].Remove(0, "/eventqueue/events/%d", i);
			i -= 1;
			count -= 1;
			// TODO: Wrap ServiceEvent with Call_StartFunction(INVALID_HANDLE, ServiceEvent)...
			//       That will help with errors that are thrown...
			ServiceEvent(event, dbgprint);
		}
		else
		{
			gJ_Timer[client].SetF32(
				delay,
				0,
				"/eventqueue/events/%d/delay",
				i
			);
		}
	}
}

#if DO_CMD_POST
public Action SRCWR_OnPlayerRunCmdPost(int client)
#else
public void OnPlayerRunCmdPre(int client)
#endif
{
	Runner(client);
#if DO_CMD_POST
	return Plugin_Changed;
#endif
}

//////////////////////////////////////////////////////////////////////////
//
//////////////////////////////////////////////////////////////////////////

//Credits to gammacase for this workaround.
int EntityToBCompatRef(Address player)
{
	if(player == Address_Null)
		return INVALID_EHANDLE_INDEX;

	int m_RefEHandle = LoadFromAddress(player + view_as<Address>(g_iRefOffset), NumberType_Int32);

	if(m_RefEHandle == INVALID_EHANDLE_INDEX)
		return INVALID_EHANDLE_INDEX;

	// https://github.com/perilouswithadollarsign/cstrike15_src/blob/29e4c1fda9698d5cebcdaf1a0de4b829fa149bf8/public/basehandle.h#L137
	int entry_idx = m_RefEHandle & ENT_ENTRY_MASK;

	if(entry_idx >= MAX_EDICTS)
		return m_RefEHandle | (1 << 31);

	return entry_idx;
}

public MRESReturn DHook_AddEventThree(Handle hParams)
{
	event_t ev;
	ev.activator = EntityToBCompatRef(view_as<Address>(DHookGetParam(hParams, 5)));
	int client = EntRefToEntIndex(ev.activator);

	if (client < 1 || client > MaxClients)
	{
		return MRES_Ignored;
	}

	DHookGetParamString(hParams, 1, ev.target, sizeof(ev.target));
	DHookGetParamString(hParams, 2, ev.targetInput, sizeof(ev.targetInput));
	ResolveVariantValue(hParams, 3, ev.variantValue);

	int ticks = RoundToCeil((view_as<float>(DHookGetParam(hParams, 4)) - FLT_EPSILON) / GetTickInterval());
	ev.delay = float(ticks);
	ev.caller = EntityToBCompatRef(view_as<Address>(DHookGetParam(hParams, 6)));
	ev.outputID = DHookGetParam(hParams, 7);

	if (gJ_Settings[client].GetBool(0, "/eventqueue/debug"))
	{
		PrintToChat(client, "[%i] AddEventThree: %s, %s, %s, %f, %i, %i, time: %f", GetGameTickCount(), ev.target, ev.targetInput, ev.variantValue, ev.delay, EntRefToEntIndex(ev.caller), ev.outputID, GetGameTime());
	}

	gJ_Timer[client].SetStruct(ev, event_t_fmt, 0, "/eventqueue/events/-1");
	return MRES_Supercede;
}

public void ResolveVariantValue(Handle &params, int offset, char variantValue[256])
{
	int type = DHookGetParamObjectPtrVar(params, offset, 16, ObjectValueType_Int);

	switch(type)
	{
		// Float
		case 1:
		{
			float fVar = DHookGetParamObjectPtrVar(params, offset, 0, ObjectValueType_Float);

			// Type recognition is difficult, even for valve programmers. Sometimes floats are integers, lets fix that.
			if (FloatAbs(fVar - RoundFloat(fVar)) < 0.000001)
				IntToString(RoundFloat(fVar), variantValue, sizeof(variantValue));
			else
				FloatToString(fVar, variantValue, sizeof(variantValue));
		}

		// Integer
		case 5:
		{
			int iVar = DHookGetParamObjectPtrVar(params, offset, 0, ObjectValueType_Int);
			IntToString(iVar, variantValue, sizeof(variantValue));
		}

		default:
		{
			DHookGetParamObjectPtrString(params, offset, 0, ObjectValueType_String, variantValue, sizeof(variantValue));
		}
	}
}

public MRESReturn DHook_ActivateMultiTrigger(int pThis, DHookParam hParams)
{
	int client = hParams.Get(1);

	if(!(0 < client <= MaxClients) || !IsClientInGame(client) || IsFakeClient(client))
		return MRES_Ignored;

	float m_flWait = GetEntPropFloat(pThis, Prop_Data, "m_flWait");

	for (int i = 0; ; ++i)
	{
		int caller = gJ_Timer[client].GetCell(0, "/eventqueue/outputwaits/%d/caller", i);
		if (caller == 0)
			break;
		if (pThis == EntRefToEntIndex(caller))
			return MRES_Supercede;
	}

	char hello[69];
	FormatEx(hello, sizeof(hello),
		"{\"caller\": %d, \"waitTime\": %d}",
		EntIndexToEntRef(pThis),
		RoundToCeil((m_flWait - FLT_EPSILON) / GetTickInterval())
	);
	gJ_Timer[client].SetFromString(hello, 0, 0, "/eventqueue/outputwaits/-1");
	SetEntProp(pThis, Prop_Data, "m_nNextThinkTick", 0);

	return MRES_Ignored;
}

int FindEntityByName(int startEntity, char[] targetname, int searchingEnt, int activator, int caller)
{
	Address targetEntityAddr = SDKCall(g_hFindEntityByName, startEntity, targetname, searchingEnt, activator, caller, 0);

	if(targetEntityAddr == Address_Null)
		return -1;

	return EntRefToEntIndex(EntityToBCompatRef(targetEntityAddr));
}

public void ServiceEvent(event_t event, bool dbgprint)
{
	int targetEntity = -1;

	int caller = EntRefToEntIndex(event.caller);
	int activator = EntRefToEntIndex(event.activator);

	if(!IsValidEntity(caller))
		caller = -1;

	// In the context of the event, the searching entity is also the caller
	while ((targetEntity = FindEntityByName(targetEntity, event.target, caller, activator, caller)) != -1)
	{
		SetVariantString(event.variantValue);
		AcceptEntityInput(targetEntity, event.targetInput, activator, caller, event.outputID);

		if (dbgprint)
		{
			PrintToChat(activator, "[%i] Performing output: %s, %i, %i, %s %s, %i, %f", GetGameTickCount(), event.target, targetEntity, caller, event.targetInput, event.variantValue, event.outputID, GetGameTime());
		}
	}
}
