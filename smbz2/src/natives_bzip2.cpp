// SPDX-License-Identifier: GPL-3.0-or-later
// Copyright 2024 rtldg <rtldg@protonmail.com>
// This file is part of srcwrtimer (https://github.com/srcwr/srcwrtimer/)


#include "../../extshared/src/extension.h"
#include "../../extshared/src/coreident.hpp"
#include "rust_exports_bzip2.h"


extern const sp_nativeinfo_t BZ2Natives[];


void MyExtension::OnHandleDestroy(HandleType_t type, void* object) {}
bool MyExtension::GetHandleApproxSize(HandleType_t type, void* object, unsigned int* size) { return false; }


bool Extension_OnLoad(char* error, size_t maxlength)
{
	sharesys->AddNatives(myself, BZ2Natives);
	return true;
}

void Extension_OnUnload()
{
	// NOTE: We will crash if unloaded while an compress/uncompress action is running... lol
}

void Extension_OnAllLoaded() {}

static cell_t N_BZ2_XompressFile(IPluginContext* ctx, const cell_t* params)
{
	char *infile, *outfile, infilebuf[PLATFORM_MAX_PATH], outfilebuf[PLATFORM_MAX_PATH];
	(void)ctx->LocalToString(params[1], &infile);
	smutils->BuildPath(Path_Game, infilebuf, sizeof(infilebuf), "%s", infile);
	(void)ctx->LocalToString(params[2], &outfile);
	smutils->BuildPath(Path_Game, outfilebuf, sizeof(outfilebuf), "%s", outfile);

	cell_t compressionLevel = params[0]==4 ? -1 : params[3];

	IPluginFunction* callback;

	if (!(callback = ctx->GetFunctionById(params[params[0]==4 ? 3 : 4])))
		return ctx->ThrowNativeError("failed to get callback function");

	IChangeableForward* forward = forwards->CreateForwardEx(
		  NULL
		, ET_Ignore
		, 4
		, NULL
		, Param_Cell
		, Param_String
		, Param_String
		, Param_Cell
	);

	if (forward == NULL)
		return ctx->ThrowNativeError("failed to create callback forward");
	if (!forward->AddFunction(callback)) {
		forwards->ReleaseForward(forward);
		return ctx->ThrowNativeError("failed to add callback function to forward");
	}

	cell_t data = params[params[0]==4 ? 4 : 5];

	return rust_BZ2_XompressFile(infile, outfile, infilebuf, outfilebuf, compressionLevel, forward, data);
}

extern const sp_nativeinfo_t BZ2Natives[] = {
	{"BZ2_DecompressFile", N_BZ2_XompressFile},
	{"BZ2_CompressFile", N_BZ2_XompressFile},
	{NULL, NULL}
};
