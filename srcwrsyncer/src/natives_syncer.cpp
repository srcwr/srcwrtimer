// SPDX-License-Identifier: GPL-3.0-or-later
// Copyright 2022 rtldg <rtldg@protonmail.com>
// This file is part of srcwrtimer (https://github.com/srcwr/srcwrtimer/)

#include "../../extshared/src/extension.h"
#include "rust_exports_syncer.h"

extern const sp_nativeinfo_t SyncerNatives[];

bool Extension_OnLoad(char* error, size_t maxlength)
{
	sharesys->AddNatives(myself, SyncerNatives);
	return true;
}

void Extension_OnUnload() {}

void Extension_OnAllLoaded() {}

cell_t N_SRCWR_Syncer_action(IPluginContext* ctx, const cell_t* params, bool is_clone_else_pull)
{
	IPluginFunction* callback;

	if (!(callback = ctx->GetFunctionById(params[2])))
		return ctx->ThrowNativeError("failed to get callback function");

	char* thing;
	char* path;
	ctx->LocalToString(params[1], &thing);
	ctx->LocalToString(params[2], &path);

	char pathbuf[PLATFORM_MAX_PATH];
	smutils->BuildPath(Path_Game, pathbuf, sizeof(pathbuf), "%s", path);

	IChangeableForward* forward = forwards->CreateForwardEx(
		  NULL
		, ET_Ignore
		, 2
		, NULL
		, Param_Cell
		, Param_String
	);

	if (forward == NULL)
		return ctx->ThrowNativeError("failed to create callback forward");
	if (!forward->AddFunction(callback)) {
		forwards->ReleaseForward(forward);
		return ctx->ThrowNativeError("failed to add callback function to forward");
	}

	rust_SRCWR_Syncer_action(thing, path, forward, params[4], is_clone_else_pull);
	return 0;
}

static cell_t N_SRCWR_Syncer_CloneRepo(IPluginContext* ctx, const cell_t* params)
{
	return N_SRCWR_Syncer_action(ctx, params, true);
}

static cell_t N_SRCWR_Syncer_PullRepo(IPluginContext* ctx, const cell_t* params)
{
	return N_SRCWR_Syncer_action(ctx, params, false);
}

extern const sp_nativeinfo_t SyncerNatives[] = {
	{"SRCWR_Syncer_CloneRepo", N_SRCWR_Syncer_CloneRepo},
	{"SRCWR_Syncer_PullRepo", N_SRCWR_Syncer_PullRepo},
	{NULL, NULL}
};
