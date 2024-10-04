// SPDX-License-Identifier: GPL-3.0-or-later
// Copyright 2022 rtldg <rtldg@protonmail.com>
// This file is part of srcwrtimer (https://github.com/srcwr/srcwrtimer/)

#if defined _srcwr_syncer_included
	#endinput
#endif
#define _srcwr_syncer_included

typeset SRCWR_Syncer_Callback {
	function void(any data, const char[] error);
}

native void SRCWR_Syncer_CloneRepo(const char[] url, const char[] path, SRCWR_Syncer_Callback callback, any data);
native void SRCWR_Syncer_PullRepo(const char[] remote_branch, const char[] path, SRCWR_Syncer_Callback callback, any data);

public Extension __ext_srcwrsyncer =
{
	name = "srcwrsyncer",
	file = "srcwrsyncer.ext",
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
public void __ext_srcwrsyncer_SetNTVOptional()
{
	MarkNativeAsOptional("SRCWR_Syncer_CloneRepo");
	MarkNativeAsOptional("SRCWR_Syncer_PullRepo");
}
#endif
