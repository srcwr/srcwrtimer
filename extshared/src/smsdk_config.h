// SPDX-License-Identifier: GPL-3.0-or-later
// Copyright 2022 rtldg <rtldg@protonmail.com>
// This file is part of srcwrtimer (https://github.com/srcwr/srcwrtimer/)

#pragma once

#define SMEXT_CONF_NAME			"SRCWR extshared name (THIS ISN'T USED)"
#define SMEXT_CONF_DESCRIPTION	"SRCWR extshared description (THIS ISN'T USED)"
#define SMEXT_CONF_VERSION		"1.0.0 (THIS ISN'T USED)"
#define SMEXT_CONF_AUTHOR		"rtldg (THIS ISN'T USED)"
#define SMEXT_CONF_URL			"https://github.com/srcwr/srcwrtimer (THIS ISN'T USED)"
#define SMEXT_CONF_LOGTAG		"SRCWR (THIS ISN'T USED)"
#define SMEXT_CONF_LICENSE		"GPL-3.0-or-later (THIS ISN'T USED)"
#define SMEXT_CONF_DATESTRING	__DATE__

#ifndef SOURCE_ENGINE
#define META_NO_HL2SDK
#endif
#define SMEXT_CONF_METAMOD
#define SMEXT_ENABLE_FORWARDSYS
#define SMEXT_ENABLE_HANDLESYS
#define SMEXT_ENABLE_PLAYERHELPERS
#define SMEXT_ENABLE_DBMANAGER
#define SMEXT_ENABLE_GAMECONF
#define SMEXT_ENABLE_MEMUTILS
#define SMEXT_ENABLE_GAMEHELPERS
#define SMEXT_ENABLE_TIMERSYS
#define SMEXT_ENABLE_THREADER
#define SMEXT_ENABLE_LIBSYS
#ifndef META_NO_HL2SDK
#define SMEXT_ENABLE_MENUS
#endif
#define SMEXT_ENABLE_ADTFACTORY
#define SMEXT_ENABLE_PLUGINSYS
#define SMEXT_ENABLE_ADMINSYS
#define SMEXT_ENABLE_TEXTPARSERS
#ifndef META_NO_HL2SDK
#define SMEXT_ENABLE_USERMSGS
#endif
#define SMEXT_ENABLE_TRANSLATOR
#define SMEXT_ENABLE_ROOTCONSOLEMENU
