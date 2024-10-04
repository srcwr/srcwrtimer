// SPDX-License-Identifier: GPL-3.0-or-later
// Copyright 2024 rtldg <rtldg@protonmail.com>
// This file is part of srcwrtimer (https://github.com/srcwr/srcwrtimer/)

#if defined _srcwr_sample_included
	#endinput
#endif
#define _srcwr_sample_included
#pragma semicolon 1


native void Sample_GetWindowsInfo(char[] buf, int outbuflen);


public Extension __ext_srcwrsample =
{
	name = "srcwrsample",
	file = "srcwrsample.ext",
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
public void __ext_srcwrsample_SetNTVOptional()
{
	MarkNativeAsOptional("Sample_GetWindowsInfo");
}
#endif
