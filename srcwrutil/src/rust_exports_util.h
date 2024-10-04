// SPDX-License-Identifier: GPL-3.0-or-later
// Copyright 2022 rtldg <rtldg@protonmail.com>
// This file is part of srcwrtimer (https://github.com/srcwr/srcwrtimer/)

#pragma once

#include <stddef.h>

class IFileObject;

typedef unsigned bigbool;

extern "C" {

void rust_handle_destroy_SmolStringList(void* object);
bool rust_handle_size_SmolStringList(void* object, unsigned int* size);

int rust_SRCWRUTIL_GetSHA1_File(IFileObject* fileobject, char* buffer);
// This one is not intended for use from natives...
int rust_SRCWRUTIL_GetSHA1_FilePath(const char* filename, char* buffer);

}
