// SPDX-License-Identifier: GPL-3.0-or-later
// Copyright 2022-2024 rtldg <rtldg@protonmail.com>
// This file is part of srcwrtimer (https://github.com/srcwr/srcwrtimer/)

#if defined _srcwr_json_included
	#endinput
#endif
#define _srcwr_json_included
#pragma semicolon 1


/*
Welcome to a utilitarian JSON API!

JSON objects and arrays are in the same `type`: SRCWRJSON.
This is much less annoying than ripext.

You don't get any good errors. Attempting to get with a key that doesn't exist: you get `0`.
This is convenient for writing SRCWRJSON since I don't have to do much error-handling.

By default key will attempt to resolve as "JSON pointers".
This allows easier access of nested JSON values with less object cloning.
	Syntax JSON pointers provided futher below...

Most method parameters are also ordered so we can format keys/values with varargs.
They are usually ordered something like one of these:
	- `buf, buflen, flags, keyfmt, varags...`
	- `idx, flags, keyfmt, varargs...`

Idx (index) functions and JSON pointers can use negative numbers to access arrays from the end.
	- `json.SetBoolIdx(true, 0, -1)` push append `true` to an array...
	- `json.GetBoolIdx(0, -2)` will return `true` from [false, true, false]
	- `json.GetBool(0, "/example-object/example-array/-3")` will return `true` from `{"example-object": { "example-array": [false, false, true, false, false] } }`
*/

// Fuck empty string keys ("")!
// Fuck string formats that embed NUL characters!
//   SRCWRJSON.FromString2(0, "test%c", '\0');
//   ^^^ Don't try to format strings to end early with NUL in any of these format functions!
//       Why? Because the count of formatted chars is passed, instead of strlen(buf)!

// TODO: Builder API where each SetX() returns the handle...

// TODO: Add multiple examples for every function!!!!!


// TODO: Make these an enum or something...
#define J_SERVER_ONLY         (      0)
#define J_ALL_CLIENTS         (1 <<  0)
#define J_PRETTY              (1 <<  1)
#define J_DONT_NULL_TERMINATE (1 <<  2)
//#define J_THROW_ON_BAD_WRITE  (1 <<  3)
//#define J_TOSTRING_NOALLOC    (1 <<  4)
#define J_UNCHECKED_UTF8_KEY  (1 <<  5)
#define J_UNCHECKED_UTF8_VAL  (1 << 27) // TODO: Remove?
//#define J_UNSIGNED            (1 <<  6)
//#define J_RAW_BYTES           (1 <<  7)
#define J_PATH                (1 <<  8)
#define J_NO_POINTER          (1 <<  9)
//#define J_ADD                 (1 << 10)
//#define J_SUB                 (1 << 11)
//#define J_MUL                 (1 << 12)
//#define J_DIV                 (1 << 13)
#define J_IS_DIFFERENT        (1 << 14) // makes set-functions return true if diff
#define J_FILE_CREATE         (1 << 15)
#define J_FILE_TRUNCATE       (1 << 16)
#define J_FILE_APPEND         (1 << 17)
#define J_FILE_NULL_TERMINATE (1 << 18)
#define J_FILE_STOP_AT_ZERO   (1 << 19)
#define J_JSON_LINES          (1 << 20)
//#define J_FULL_POINTER        (1 << 21)
#define J_CREATE_PARENTS      (1 << 22)
#define J_MERGE               (1 << 23)
#define J_SWAP_REMOVE         (1 << 24)
// Get: takes the value out and replaces it with null...
// Set: puts the value in and makes the the original value be null...
// TODO: clarify because those descriptions suck...
#define J_MOVE                (1 << 26)

enum J_Type {
	J_Unknown = 0,
	J_Null = 1,
	J_Bool = 2,
	J_Number = 3,
	J_String = 4,
	J_Array = 5,
	J_Map = 6,
}
char gS_J_Type[][] = {
	"unknown",
	"null",
	"bool",
	"number",
	"string",
	"array",
	"map",
};
#pragma unused gS_J_Type


methodmap SRCWRJSON < Handle {
	// Creates a new JSON value. {} by default. [] if `array` is true.
	public native SRCWRJSON(bool array = false);
	// Duplicates the JSON value and returns a new handle to it.
	public native SRCWRJSON Clone();

	// Creates JSON objects in a batch that can be reobtained with the same `name` (like in another plugin).
	public static native bool GetHandles(const char[] name, SRCWRJSON[] handles, int count, bool owned_by_extension=true);

	// Writes the JSON value to a file...
	public native bool ToFile(const char[] file,
		int flags = J_PRETTY | J_FILE_CREATE | J_FILE_TRUNCATE,
		const char[] key = NULL_STRING, any ...);
	// Writes the JSON value to a file (at its current file position).
	public native bool ToFileHandle(File file, int flags = J_PRETTY, const char[] key = NULL_STRING, any ...);

	public native int ToString(char[] buffer, int maxlength, int flags = 0, const char[] key = NULL_STRING, any ...);

	public void ToServer(int flags = J_PRETTY, const char[] key = NULL_STRING, any ...)
	{
		char buf[1024];
		bool nullstr = IsNullString(key);
		if (!nullstr) VFormat(buf, sizeof(buf), key, 3);
		this.ToString(buf, sizeof(buf), flags, nullstr ? NULL_STRING : buf);
		PrintToServer("%s", buf);
	}
	public void ToChat(int client, int flags = 0, const char[] key = NULL_STRING, any ...)
	{
		char buf[253]; // strlen(buf)>252 is when CSS stops sending messages
		bool nullstr = IsNullString(key);
		if (!nullstr) VFormat(buf, sizeof(buf), key, 4);
		this.ToString(buf, sizeof(buf), flags, nullstr ? NULL_STRING : buf);
		PrintToChat(client, "%s", buf);
	}
	public void ToChats(int[] clients, int count, int flags = 0, const char[] key = NULL_STRING, any ...)
	{
		char buf[253]; // strlen(buf)>252 is when CSS stops sending messages
		bool nullstr = IsNullString(key);
		if (!nullstr) VFormat(buf, sizeof(buf), key, 5);
		this.ToString(buf, sizeof(buf), flags, nullstr ? NULL_STRING : buf);
		for (int i = 0; i < count; ++i)
		{
			PrintToChat(clients[i], "%s", buf);
		}
	}
	public void ToConsole(int client, int flags = 0, const char[] key = NULL_STRING, any ...)
	{
		char buf[253]; // TODO: buf limit for console?
		bool nullstr = IsNullString(key);
		if (!nullstr) VFormat(buf, sizeof(buf), key, 4);
		this.ToString(buf, sizeof(buf), flags, nullstr ? NULL_STRING : buf);
		PrintToConsole(client, "%s", buf);
	}
	public void ToConsoles(int[] clients, int count, int flags = 0, const char[] key = NULL_STRING, any ...)
	{
		char buf[253]; // TODO: buf limit for console?
		bool nullstr = IsNullString(key);
		if (!nullstr) VFormat(buf, sizeof(buf), key, 5);
		this.ToString(buf, sizeof(buf), flags, nullstr ? NULL_STRING : buf);
		for (int i = 0; i < count; ++i)
		{
			PrintToConsole(clients[i], "%s", buf);
		}
	}

	//public native int ToStringMalloc(int flags = 0);
	//public static native void FreeMalloced(int ptr);
	// used when passing json object -> sql / replay and such...
	// a little sussy if extensions are using different json object layouts...
	//public native int ToUnsafePtr();

	public static native SRCWRJSON FromFile(const char[] file, int flags = 0);
	public static native SRCWRJSON FromFileHandle(File file, int flags = 0);
	public static native SRCWRJSON FromString1(const char[] buffer, int end = 0, int flags = 0);
	public static native SRCWRJSON FromString2(int flags = 0, const char[] fmt, any ...);

	public native bool Has(int flags, const char[] key, any ...);

	// Returns J_Unknown if fails or if the key is missing...
	public native J_Type GetType(int flags = 0, const char[] key, any ...);
	property J_Type Type {
		public native get();
	}

	// TODO: Remove? Make like `ToChat` and not be a native, but instead a .... whatever that'd be...?
	public native bool IsArray(int flags = 0, const char[] key, any ...);

	public native int len(int flags = 0, const char[] key, any ...);
	property int Length {
		public native get();
	}

	// TODO: ArrayList-esque functions...
	//public native int FindCell(const char[] key, any value, int flags = 0);
	//public native int FindString(const char[] key, )

	public native SRCWRJSON Get(int flags, const char[] key = NULL_STRING, any ...);
	public native SRCWRJSON GetIdx(int flags, int idx);
	public native bool Set(SRCWRJSON value, int flags, const char[] key = NULL_STRING, any ...);
	public native bool SetIdx(SRCWRJSON value, int flags, int idx);

	// end < 1 for strlen end...
	public native bool SetFromString(const char[] value, int end = 0, int flags = 0, const char[] key = NULL_STRING, any ...);
	public native bool SetFromStringIdx(const char[] value, int end, int flags, int idx);

	public native bool Remove(int flags, const char[] key, any ...);
	public native bool RemoveIdx(int flags, int idx);

	// selector uses jsonpath syntax
	//public native int RemoveAllWithSelector(const char[] selectorpath, int flags, const char[] key, any ...);

	public native bool Clear(int flags = 0, const char[] key = NULL_STRING, any ...);

	// TODO: Remove?
	// Replaces the cell & returns the old value.
	// Note: returns 0 on missing key or non-number type...
	public native int ReplaceCell(int newcell, int flags, const char[] key, any ...);
	public native int ReplaceCellIdx(int newcell, int flags, int idx);

	// Copy things from other things....
	public native bool SetZss(int flags=0, const char[] key, SRCWRJSON other, const char[] key2, any ...);
#if 0
	public native bool SetZsi(const char[] key, SRCWRJSON other, int key2, int flags = 0);
	public native bool SetZis(int key, SRCWRJSON other, const char[] key2, int flags = 0);
	public native bool SetZii(const char[] key, SRCWRJSON other, int key2, int flags = 0);
#endif

	/*
	enum struct testo {
		int authid;
		float time;
		char sss[32];
		bool testy;
		int wew[2];
	}

	// '{"authid": 123123123, "time": 123.123, "sss": "abc123", "testy": true, "wew": [123,456] }'
	char[] thing = "{ \"authid\": 123123123, \"time\": 123.123, \"sss\": \"abc123\", \"testy\": true, \"wew\": [123,456] }";
	char[] fmt = "authid,int|time,float|sss,char,32|testy,bool|wew,int,2";

	SRCWRJSON json = SRCWRJSON.FromString1(thing);

	testo test;
	json.GetStruct(test, fmt, 0);
	PrintToServer("%d %f '%s' %d [%d,%d]", test.authid, test.time, test.sss, test.testy, test.wew[0], test.wew[1]);

	// 123123123 123.123001 'abc123' 1 [123,456]
	*/
	public native bool GetStruct(any[] buf, const char[] format, int flags = 0, const char[] key = NULL_STRING, any ...);
	public native bool GetStructIdx(any[] buf, const char[] format, int flags = 0, int idx);
	public native bool SetStruct(const any[] buf, const char[] format, int flags = 0, const char[] key = NULL_STRING, any ...);
	public native bool SetStructIdx(const any[] buf, const char[] format, int flags = 0, int idx);

	// Returns 0 if the key/idx couldn't be found / wasn't an int / convertable
	public native any GetCell(int flags = 0, const char[] key, any ...);
	public native any GetCellIdx(int flags, int idx);
	// Returns true if the value could be set
	public native bool SetCell(any value, int flags, const char[] key, any ...);
	public native bool SetCellIdx(int flags, int idx, any value);

	public native bool GetCellArray(const char[] key, any[] buf, int count, int offset = 0, int flags = 0);
	public native bool GetCellArrayIdx(const char[] key, int count, int offset = 0, int flags = 0);
	public native bool SetCellArray(const char[] key, any[] buf, int count, int offset = 0, int flags = 0);
	public native bool SetCellArrayIdx(int idx, any[] buf, int count, int offset = 0);

	// Returns 0.0 if the key/idx couldn't be found / wasn't a float / convertable
	public native float GetF32(int flags = 0, const char[] key, any ...);
	public native float GetF32Idx(int flags = 0, int idx);
	// Returns true if the key/idx could be set
	public native bool SetF32(float value, int flags, const char[] key, any ...);
	public native bool SetF32Idx(float value, int flags, int idx);

	// Returns false for non-bools / non-existent keys.
	public native bool GetBool(int flags, const char[] key, any ...);
	public native bool GetBoolIdx(int flags, int idx);
	// Returns true if the value was set.
	public native bool SetBool(bool value, int flags, const char[] key, any ...);
	public native bool SetBoolIdx(bool value, int flags, int idx);
	// Returns the new bool value. Returns false if couldn't set the key...
	// Idk...
	public native bool ToggleBool(int flags, const char[] key, any ...);
	public native bool ToggleBoolIdx(int flags, int idx);

	// Returns 0.0 if couldn't access key/idx as f64.
	// Returns f64 truncated to f32 value on success (and fills buffer too)
	public native float GetF64(any[] buffer, int flags = 0, const char[] key, any ...);
	public native float GetF64Idx(any[] buffer, int flags = 0, int idx);
	// Returns true if the key/idx could be set.
	public native bool SetF64(any[] value, int flags = 0, const char[] key, any ...);
	public native bool SetF64Idx(any[] value, int flags = 0, int idx);

	// Returns 0 if couldn't access key/idx as i64.
	// Returns i64 truncated to i32 value on success (and fills buffer too)
	public native int GetI64(any[] buffer, int flags = 0, const char[] key, any ...);
	public native int GetI64Idx(any[] buffer, int flags = 0, int idx);
	// Returns true if the value could be set
	public native bool SetI64(const any[] value, int flags = 0, const char[] key, any ...);
	public native bool SetI64Idx(const any[] value, int flags = 0, int idx);
	//public native bool SetI64ss(const char[] key, const char[] value, int flags = 0);
	//public native bool SetI64Idx(int idx, const char[] value, int flags = 0);

#if 0
	// TODO:
	public native bool GetI64Array(const char[] key, any[] buffer, int flags = 0);
	public native bool GetI64ArrayIdx(int idx, any[] buffer);
	public native bool SetI64Array(const char[] key, const any[] array);
	public native bool SetI64ArrayIdx(int idx, const any[] array);
#endif

	// Returns false if the key/idx couldn't be found
	public native bool IsNull(int flags, const char[] key, any ...);
	public native bool IsNullIdx(int flags, int idx);
	// Returns true if the value could be set to null
	public native bool SetNull(int flags, const char[] key, any ...);
	public native bool SetNullIdx(int flags, int idx);

	// Returns number of bytes/chars written to buffer, NOT including the null terminator.
	// Returns 0 if buffer is too small or the value isn't a string...
	public native int GetString(char[] buffer, int maxlength, int flags, const char[] key, any ...);
	public native int GetStringIdx(int idx, char[] buffer, int maxlength, int flags = 0);
	// Returns true if the key/idx could be set.
	public native bool SetString(const char[] value, int end = 0, int flags = 0, const char[] key, any ...);
	public native bool SetStringIdx(int idx, const char[] value, int end = 0, int flags = 0);
	// public native bool SetString2(const char[] key, const any[] value, int end, int flags = 0);
	// public native bool SetString2Idx(int idx, const any[] value, int end, int flags = 0);
	// append to string

	/*
native int SetI32Array(int client, const char[] key, any[] value, int len);
native int SetI64Array(int client, const char[] key, any[][] value, int len);

native int SetF32Array(int client, const char[] key, any[] value, int len);
native int SetF64Array(int client, const char[] key, any[][] value, int len);

native int AddI32(int client, const char[] key, any value);
native int AddI64(int client, const char[] key, any[] value);

native int AddF32(int client, const char[] key, any value);
native int AddF64(int client, const char[] key, any[] value);
	*/

	// Example:
	/*
	ArrayList keys = new ArrayList(ByteCountToCells(32));
	if (json.FillKeys(keys, 0, "/3/actor"))
	{
		for (int i = 0, len = keys.Length; i < len; ++i)
		{
			char buf[32];
			keys.GetString(i, buf, sizeof(buf));
			PrintToServer("%d = '%s'", i, buf);
		}
	}
	delete keys;
	*/
	public native bool FillKeys(ArrayList keys, int flags = 0, const char[] key = NULL_STRING, any ...);
}


public Extension __ext_srcwrjson = {
	name = "srcwrjson",
	file = "srcwrjson.ext",
#if defined AUTOLOAD_EXTENSIONS
	autoload = 1,
#else
	autoload = 0,
#endif
#if defined REQUIRE_EXTENSIONS
	required = 1,
#else
	required = 0,
#endif
};

#if !defined REQUIRE_EXTENSIONS
public void __ext_srcwrjson_SetNTVOptional()
{
	MarkNativeAsOptional("SRCWRJSON.SRCWRJSON");
	MarkNativeAsOptional("SRCWRJSON.Clone");

	MarkNativeAsOptional("SRCWRJSON.GetHandles");

	MarkNativeAsOptional("SRCWRJSON.ToFile");
	MarkNativeAsOptional("SRCWRJSON.ToFileHandle");

	MarkNativeAsOptional("SRCWRJSON.ToString");

	//MarkNativeAsOptional("SRCWRJSON.ToStringMalloc");
	// MarkNativeAsOptional("SRCWRJSON.FreeMalloced");
	// MarkNativeAsOptional("SRCWRJSON.ToUnsafePtr");

	MarkNativeAsOptional("SRCWRJSON.FromFile");
	MarkNativeAsOptional("SRCWRJSON.FromFileHandle");
	MarkNativeAsOptional("SRCWRJSON.FromString1");
	MarkNativeAsOptional("SRCWRJSON.FromString2");

	MarkNativeAsOptional("SRCWRJSON.Has");
	MarkNativeAsOptional("SRCWRJSON.IsArray");
	MarkNativeAsOptional("SRCWRJSON.len");
	MarkNativeAsOptional("SRCWRJSON.Length.get");

	MarkNativeAsOptional("SRCWRJSON.Get");
	MarkNativeAsOptional("SRCWRJSON.GetIdx");
	MarkNativeAsOptional("SRCWRJSON.Set");
	MarkNativeAsOptional("SRCWRJSON.SetIdx");

	MarkNativeAsOptional("SRCWRJSON.SetFromString");
	MarkNativeAsOptional("SRCWRJSON.SetFromStringIdx");

	MarkNativeAsOptional("SRCWRJSON.Remove");
	MarkNativeAsOptional("SRCWRJSON.RemoveIdx");

	MarkNativeAsOptional("SRCWRJSON.Clear");

	MarkNativeAsOptional("SRCWRJSON.ReplaceCell");
	MarkNativeAsOptional("SRCWRJSON.ReplaceCellIdx");

	MarkNativeAsOptional("SRCWRJSON.SetZss");

	MarkNativeAsOptional("SRCWRJSON.GetStruct");
	MarkNativeAsOptional("SRCWRJSON.GetStructIdx");
	MarkNativeAsOptional("SRCWRJSON.SetStruct");
	MarkNativeAsOptional("SRCWRJSON.SetStructIdx");

	MarkNativeAsOptional("SRCWRJSON.GetCell");
	MarkNativeAsOptional("SRCWRJSON.GetCellIdx");
	MarkNativeAsOptional("SRCWRJSON.SetCell");
	MarkNativeAsOptional("SRCWRJSON.SetCellIdx");

	MarkNativeAsOptional("SRCWRJSON.GetCellArray");
	MarkNativeAsOptional("SRCWRJSON.GetCellArrayIdx");
	MarkNativeAsOptional("SRCWRJSON.SetCellArray");
	MarkNativeAsOptional("SRCWRJSON.SetCellArrayIdx");

	MarkNativeAsOptional("SRCWRJSON.GetF32");
	MarkNativeAsOptional("SRCWRJSON.GetF32Idx");
	MarkNativeAsOptional("SRCWRJSON.SetF32");
	MarkNativeAsOptional("SRCWRJSON.SetF32Idx");

	MarkNativeAsOptional("SRCWRJSON.GetBool");
	MarkNativeAsOptional("SRCWRJSON.GetBoolIdx");
	MarkNativeAsOptional("SRCWRJSON.SetBool");
	MarkNativeAsOptional("SRCWRJSON.SetBoolIdx");
	MarkNativeAsOptional("SRCWRJSON.ToggleBool");
	MarkNativeAsOptional("SRCWRJSON.ToggleBoolIdx");

	MarkNativeAsOptional("SRCWRJSON.GetF64");
	MarkNativeAsOptional("SRCWRJSON.GetF64Idx");
	MarkNativeAsOptional("SRCWRJSON.SetF64");
	MarkNativeAsOptional("SRCWRJSON.SetF64Idx");

	MarkNativeAsOptional("SRCWRJSON.GetI64");
	MarkNativeAsOptional("SRCWRJSON.GetI64Idx");
	MarkNativeAsOptional("SRCWRJSON.SetI64");
	MarkNativeAsOptional("SRCWRJSON.SetI64Idx");

	MarkNativeAsOptional("SRCWRJSON.IsNull");
	MarkNativeAsOptional("SRCWRJSON.IsNullIdx");
	MarkNativeAsOptional("SRCWRJSON.SetNull");
	MarkNativeAsOptional("SRCWRJSON.SetNullIdx");

	MarkNativeAsOptional("SRCWRJSON.GetString");
	MarkNativeAsOptional("SRCWRJSON.GetStringIdx");
	MarkNativeAsOptional("SRCWRJSON.SetString");
	MarkNativeAsOptional("SRCWRJSON.SetStringIdx");

	MarkNativeAsOptional("SRCWRJSON.FillKeys");
}
#endif
