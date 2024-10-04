// SPDX-License-Identifier: GPL-3.0-or-later
// Copyright 2022 rtldg <rtldg@protonmail.com>
// This file is part of srcwrtimer (https://github.com/srcwr/srcwrtimer/)

#pragma once

#include <stddef.h>

namespace SourceMod {
class IChangeableForward;
}
using namespace SourceMod;

extern "C" {

void rust_SRCWR_Syncer_action(const char* thing, const char* path, IChangeableForward* forward, int data, bool is_clone_else_pull);

}
