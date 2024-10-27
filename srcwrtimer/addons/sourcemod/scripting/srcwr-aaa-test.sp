// SPDX-License-Identifier: GPL-3.0-or-later
// Copyright 2022-2024 rtldg <rtldg@protonmail.com>

#pragma semicolon 1

#include <sourcemod>
#include <srcwr/json>
#undef REQUIRE_EXTENSIONS
#include <srcwr/http>
#include <profiler>

#undef REQUIRE_PLUGIN
#include <srcwr/core>

native float Shavit_GetStyleSettingFloat(int style, const char[] key);
native int Shavit_GetBhopStyle(int client);

Handle gT_Timer = null;

enum struct testo {
	int authid;
	float time;
	char sss[32];
	bool testy;
	int wew[2];
}
#define testo_fmt "authid,int|time,float|sss,char,32|testy,bool|wew,int,2"

Action Timer_Hey(Handle timer)
{
	PrintToServer("TIMER AAAAAAAAAAA");
	return Plugin_Continue;
}

#if 1
void CallbackReq(any value, const char[] error, SRCWRHTTPResp resp)
{
	PrintToServer("Hello from CallbackReq | %d - %X - %s", value, resp, error);
	if (resp)
	{
#if 0
		char bbb[1024];
		resp.get(bbb, sizeof(bbb));
		PrintToServer("'%s'", bbb);
		if (true) return;
#endif

		SRCWRJSON json = resp.json;
		if (json)
		{
			PrintToServer("json = %X", json);
			char buf[1024];
			json.ToString(buf, sizeof(buf), J_PRETTY);
			PrintToServer("%s", buf);
			delete json;
		}
	}
	//CreateTimer(0.5, Timer_Hey);
}
#endif

Action Command_DoReq(int client, int args)
{
#if 1
	//SRCWRHTTPReq req = new SRCWRHTTPReq("https://www.google.com");
	SRCWRHTTPReq req = new SRCWRHTTPReq("https://jsonplaceholder.typicode.com/todos/1");
	req.YEET(CallbackReq, 123);
#endif
	return Plugin_Handled;
}

Action Timer_WS(Handle timer, SRCWRWebsocket ws)
{
	PrintToServer("Sending to WS");
	ws.write_str("Hello %d", GetTime());
	return Plugin_Continue;
}

void WsConnection(any data, SRCWRWebsocket ws, const char[] close_reason)
{
	PrintToServer("WsConnection: %X %X '%s'", data, ws, close_reason);
}

void WsMsg(any data, SRCWRWebsocket ws, SRCWRWebsocketMsg msg)
{
	PrintToServer("WsMsg: %X %X %X", data, ws, msg);
	char buf[120];
	int c = msg.get(buf, sizeof(buf), 0);
	PrintToServer("> %d '%s'", c, buf);
	static int counter = 0;
	if (++counter == 3)
	{
		PrintToServer("\n\n\nHEY WE SHOULD BE DYING HERE %d\n\n\n\n", CloseHandle(ws));
		delete gT_Timer;
		//delete ws;
	}
}

Action Command_DoWs(int client, int args)
{
	SRCWRWebsocket ws = new SRCWRWebsocket();
	bool ret = ws.YEET(WsConnection, WsMsg, 0x12345678, "ws://127.0.0.1:3012");
	PrintToServer("ws = %X ret = %d", ws, ret);
	if (ret)
		gT_Timer = CreateTimer(5.0, Timer_WS, ws, TIMER_REPEAT);
	else
		delete ws;
	return Plugin_Handled;
}

Action Command_testo(int client, int args)
{
	char[] thing = "{ \"authid\": 123123123, \"time\": 123.123, \"sss\": \"abc123\", \"testy\": true, \"wew\": [123,456] }";

	SRCWRJSON json = SRCWRJSON.FromString1(thing);

	testo test;
	json.GetStruct(test, testo_fmt);
	PrintToServer("%d %f '%s' %d [%d,%d]", test.authid, test.time, test.sss, test.testy, test.wew[0], test.wew[1]);

	delete json;
	json = SRCWRJSON.FromString1("{}");
	json.SetStruct(test, testo_fmt);
	char buf[255];
	json.ToString(buf, sizeof(buf));
	PrintToServer("%s", buf);

	return Plugin_Handled;
}

public void OnPluginStart()
{
	RegConsoleCmd("sm_doreq", Command_DoReq);
	RegConsoleCmd("sm_dows", Command_DoWs);
	RegConsoleCmd("sm_testo", Command_testo);
	char error[255];
	SRCWR_GetHandles(error, sizeof(error));
	PrintToServer("'%s' %X", error, gJ_Timer[1]);

	SRCWR_SetDefaultPlayerSettings("eventqueue", "{\"debug\": false}");
	gJ_Settings[0].ToServer();

#if defined(DO_LARGE_FILE)
	SRCWRJSON json = SRCWRJSON.FromFile("large-file.json");
#if defined(DO_LARGE_FILE_PROFILING)
	Profiler profiler = new Profiler();
	profiler.Start();
	for (int i = 0; i < 10000; ++i)
	{
		//json.Has(0, "/5/repo/payload/size");
		json.SetFromString("\"fucker\"", -1, 0, "/%d/actor/test555", 3);
	}
	profiler.Stop();
	PrintToServer("time = %f", profiler.Time);
	profiler.Start();
	for (int i = 0; i < 10000; ++i)
	{
		int style = Shavit_GetBhopStyle(1);
		Shavit_GetStyleSettingFloat(1, "airaccelerate");
	}
	profiler.Stop();
	PrintToServer("time2 = %f", profiler.Time);
#endif // defined(DO_LARGE_FILE_PROFILING)
	//PrintToServer("has2 = %d", json.Has(0, "/0/actor/%s %N", "idddd", 2));
	SRCWRJSON fromstr = SRCWRJSON.FromString1("{\"t333est\": 5}");
	bool b = json.Set(fromstr, 0, "/%d/actor/test222", 3);
	bool b2 = json.SetFromString("\"fucker\"", -1, 0, "/%d/actor/test222/test4444", 3);
	//json.Clear(0, "/%d/actor/test222", 3);
	char buffer[512];
	json.ToString(buffer, sizeof(buffer), J_PRETTY, "/%d/actor", 3);
	PrintToServer("%d === %d\n%s", b, b2, buffer);
	//json.ToFile("test3.json");
#if 1
	ArrayList keys = new ArrayList(ByteCountToCells(32));
	if (json.FillKeys(keys, 0, "/%d/actor", 3))
	{
		for (int i = 0; i < keys.Length; ++i)
		{
			char buf[32];
			keys.GetString(i, buf, sizeof(buf));
			PrintToServer("%d = '%s'", i, buf);
		}
	}
	delete keys;
#endif // 1
	delete json;
	json = SRCWRJSON.FromFile("test.json");
	any myi64[2];
	char buf[255];
	PrintToServer("srcwr-aaa-test %d", json.GetI64(myi64, 0, "/0/iii"));
	Int64ToString(myi64, buf, sizeof(buf));
	PrintToServer("srcwr-aaa-test %s", buf);
	PrintToServer("srcwr-aaa-test %f", json.GetF32(0, "/0/fff"));
#endif
}
