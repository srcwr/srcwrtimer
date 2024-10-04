// SPDX-License-Identifier: GPL-3.0-or-later
// Copyright 2022 rtldg <rtldg@protonmail.com>
// This file is part of srcwrtimer (https://github.com/srcwr/srcwrtimer/)

#pragma once

extern IdentityToken_t *g_pCoreIdent;

#ifdef HANDLE_FILEOBJECT
extern HandleType_t g_FileType;
#include "IFileObject.hpp"
#endif

#ifdef HANDLE_CELLARRAY
extern HandleType_t g_ArrayListType;
#endif

bool ResolveCoreIdent(char* error, size_t maxlength);
HandleError ReadHandleCoreIdent(Handle_t handle, HandleType_t type, void** object);
