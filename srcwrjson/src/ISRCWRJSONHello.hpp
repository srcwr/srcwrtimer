// SPDX-License-Identifier: GPL-3.0-or-later
// Copyright 2022 rtldg <rtldg@protonmail.com>
// This file is part of srcwrtimer (https://github.com/srcwr/srcwrtimer/)

#pragma once

#include <IShareSys.h>

#define SMINTERFACE_JSONHELLO_NAME "SRCWRJSONHello"
#define SMINTERFACE_JSONHELLO_VERSION 1

using namespace SourceMod;

class ISRCWRJSONHello : public SMInterface
{
public:
	virtual unsigned int GetInterfaceVersion() = 0;
	virtual const char *GetInterfaceName() = 0;
public:
	virtual Handle_t MakeJSONObject(IPluginContext* ctx, const char* s, int len) = 0;
	virtual void* GetUnsafePointer(IPluginContext* ctx, cell_t handy) = 0;
};
