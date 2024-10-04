// SPDX-License-Identifier: GPL-3.0-or-later
// Copyright 2022-2024 rtldg <rtldg@protonmail.com>
// This file is part of srcwrtimer (https://github.com/srcwr/srcwrtimer/)

#include "valvefs.hpp"
#include <sm_platform.h> // PLATFORM_EXTERN_C
#include <filesystem.h> // the valve thing...

PLATFORM_EXTERN_C int valvefs_Read(FileHandle_t file, void *pOut, int size)
{
	return filesystem->Read(pOut, size, file);
}

PLATFORM_EXTERN_C int valvefs_Write(FileHandle_t file, const void *pData, int size)
{
	return filesystem->Write(pData, size, file);
}

PLATFORM_EXTERN_C void valvefs_Flush(FileHandle_t file)
{
	filesystem->Flush(file);
}

PLATFORM_EXTERN_C void valvefs_Seek(FileHandle_t file, int pos, int seek_type)
{
	filesystem->Seek(file, pos, seek_type);
}

PLATFORM_EXTERN_C unsigned int valvefs_Tell(FileHandle_t file)
{
	return filesystem->Tell(file);
}
