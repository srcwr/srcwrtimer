// SPDX-License-Identifier: GPL-3.0-or-later
// Copyright 2022 rtldg <rtldg@protonmail.com>
// This file is part of srcwrtimer (https://github.com/srcwr/srcwrtimer/)

#include "IFileObject.hpp"
#include <sm_platform.h> // PLATFORM_EXTERN_C

PLATFORM_EXTERN_C size_t IFileObject_Read(IFileObject* file, void *pOut, int size)
{
	return file->Read(pOut, size);
}

PLATFORM_EXTERN_C size_t IFileObject_Write(IFileObject* file, const void *pData, int size)
{
	return file->Write(pData, size);
}

PLATFORM_EXTERN_C bool IFileObject_Flush(IFileObject* file)
{
	return file->Flush();
}

PLATFORM_EXTERN_C bool IFileObject_Seek(IFileObject* file, int pos, int seek_type)
{
	return file->Seek(pos, seek_type);
}

PLATFORM_EXTERN_C size_t IFileObject_Tell(IFileObject* file)
{
	return (size_t)file->Tell();
}
