// SPDX-License-Identifier: GPL-3.0-or-later
// Copyright 2022 rtldg <rtldg@protonmail.com>
// This file is part of srcwrtimer (https://github.com/srcwr/srcwrtimer/)

#pragma once

#include <stddef.h>

extern "C" {

bool rust_sdk_on_load_wrapper(char* error, size_t maxlength, bool late);
void rust_sdk_on_unload();
void rust_sdk_on_all_loaded();
void rust_on_core_map_start(void* edict_list, int edict_count, int client_max);
void rust_on_core_map_end();
const char* rust_conf_name();
const char* rust_conf_description();
const char* rust_conf_version();
const char* rust_conf_author();
const char* rust_conf_url();
const char* rust_conf_logtag();
const char* rust_conf_license();

}
