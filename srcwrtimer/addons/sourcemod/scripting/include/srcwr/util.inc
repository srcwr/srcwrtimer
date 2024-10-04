// SPDX-License-Identifier: GPL-3.0-or-later
// Copyright 2022 rtldg <rtldg@protonmail.com>
// This file is part of srcwrtimer (https://github.com/srcwr/srcwrtimer/)

#if defined _srcwr_util_included
	#endinput
#endif
#define _srcwr_util_included
#pragma semicolon 1


// File handle rather than a file names because of valve filesystem search paths! FucK!
//
// `hash` should be (at least) a `char[40+1]`.
// Returns 0 if it didn't work.
native int SRCWRUTIL_GetSHA1_File(char[] hash, File file);


// Hash a string. If `datalen` is -1 then only up to `strlen(data)` bytes are hashed.
//
// `hash` should be (at least) a `char[40+1]`.
// Returns 0 if it didn't work.
native int SRCWRUTIL_GetSHA1_String(char[] hash, const char[] data, int datalen=-1);


public Extension __ext_srcwrutil =
{
	name = "srcwrutil",
	file = "srcwrutil.ext",
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
public void __ext_srcwrutil_SetNTVOptional()
{

}
#endif
