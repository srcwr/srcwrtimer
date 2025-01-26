// SPDX-License-Identifier: GPL-3.0-or-later
// Copyright 2022 rtldg <rtldg@protonmail.com>
// This file is part of srcwrtimer (https://github.com/srcwr/srcwrtimer/)

#include "../../extshared/src/extension.h"

#include <stdlib.h>

extern const sp_nativeinfo_t F64Natives[];


void MyExtension::OnHandleDestroy(HandleType_t type, void* object) {}
bool MyExtension::GetHandleApproxSize(HandleType_t type, void* object, unsigned int* size) { return false; }


bool Extension_OnLoad(char* error, size_t maxlength)
{
	sharesys->AddNatives(myself, F64Natives);

	return true;
}

void Extension_OnUnload()
{
}

void Extension_OnAllLoaded() {}

bool strcheck(IPluginContext* ctx, cell_t param, char** out)
{
	if (SP_ERROR_NONE != ctx->LocalToString(param, out))
	{
		ctx->ReportError("Invalid string local address %x passed", param);
		return false;
	}

	return true;
}

bool doublecheck(IPluginContext* ctx, cell_t param, double** out)
{
	if (SP_ERROR_NONE != ctx->LocalToPhysAddr(param, (cell_t**)out))
	{
		ctx->ReportError("Invalid double local address %x passed", param);
		return false;
	}

	return true;
}

#define QUICK(a) if (!a) return 0;

static cell_t N_F64_FromString(IPluginContext* ctx, const cell_t* params)
{
	char* s;
	double* d;

	QUICK(strcheck(ctx, params[1], &s));
	QUICK(doublecheck(ctx, params[2], &d));
	*d = atof(s);

	return 0;
}

static cell_t N_F64_ToString(IPluginContext* ctx, const cell_t* params)
{
	char* s;
	double* d;

	QUICK(strcheck(ctx, params[1], &s));
	QUICK(doublecheck(ctx, params[2], &d));
	*d = atof(s);

	return 0;
}

extern const sp_nativeinfo_t F64Natives[] = {
	{"F64_FromString", N_F64_FromString},

	{NULL, NULL}
};
