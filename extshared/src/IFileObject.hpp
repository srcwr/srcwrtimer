// SPDX-License-Identifier: GPL-3.0-only
// Copyright 2004-2014 AlliedModders LLC

#pragma once

#include <stddef.h> // size_t

// stripped out of https://github.com/alliedmodders/sourcemod/blob/master/core/logic/smn_filesystem.cpp

class IFileObject
{
public:
	virtual ~IFileObject();
	virtual size_t Read(void *pOut, int size) = 0;
	virtual char *ReadLine(char *pOut, int size) = 0;
	virtual size_t Write(const void *pData, int size) = 0;
	virtual bool Seek(int pos, int seek_type) = 0;
	virtual int Tell() = 0;
	virtual bool Flush() = 0;
	virtual bool HasError() = 0;
	virtual bool EndOfFile() = 0;
	virtual void Close() = 0;
	//virtual ValveFile *AsValveFile();
	//virtual SystemFile *AsSystemFile();
};
