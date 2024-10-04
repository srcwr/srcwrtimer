// SPDX-License-Identifier: GPL-3.0-or-later
// Copyright 2024 rtldg <rtldg@protonmail.com>
// This file is part of srcwrtimer (https://github.com/srcwr/srcwrtimer/)

#pragma once

typedef unsigned bigbool;

extern "C" {

bigbool rust_Sample_GetWindowsInfo(char* outbuf, size_t outbuflen);

}
