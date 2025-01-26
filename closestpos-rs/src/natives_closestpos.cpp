// SPDX-License-Identifier: GPL-3.0-or-later
// Copyright 2021-2024 rtldg <rtldg@protonmail.com>
// This file is part of srcwrtimer (https://github.com/srcwr/srcwrtimer/)

#include "../../extshared/src/extension.h"
#include "../../extshared/src/coreident.hpp"
#include "rust_exports_closestpos.h"

#include <vector>

template<class T> const T& Zmin(const T& a, const T& b) { return (b < a) ? b : a; }

#include <ICellArray.h>


HandleType_t g_ClosestPosType = 0;

extern const sp_nativeinfo_t ReplayNatives[];


void MyExtension::OnHandleDestroy(HandleType_t type, void* object)
{
	if (type == g_ClosestPosType) {
		rust_handle_destroy_ClosestPos(object);
	}
}
bool MyExtension::GetHandleApproxSize(HandleType_t type, void* object, unsigned int* size)
{
	if (type == g_ClosestPosType) {
		return rust_handle_size_ClosestPos(object, size);
	}
	return false;
}


bool Extension_OnLoad(char* error, size_t maxlength)
{
	sharesys->AddNatives(myself, ReplayNatives);

	g_ClosestPosType = g_pHandleSys->CreateType(
		  "ClosestPos"
		, &g_MyExtension
		, 0
		, NULL
		, NULL
		, myself->GetIdentity()
		, NULL
	);

	sharesys->RegisterLibrary(myself, "closestpos");
	return true;
}

void Extension_OnUnload()
{
	g_pHandleSys->RemoveType(g_ClosestPosType, myself->GetIdentity());
}

void Extension_OnAllLoaded() {}


static cell_t N_ClosestPos_ClosestPos(IPluginContext* ctx, const cell_t* params)
{
	ICellArray *array;
	Handle_t arraylist = params[1];
	cell_t offset = params[2];

	if (offset < 0)
		return ctx->ThrowNativeError("Offset must be 0 or greater (given %d)", offset);

	if (arraylist == BAD_HANDLE)
		return ctx->ThrowNativeError("Bad handle passed as ArrayList %x", arraylist);

	HandleError err;

	if ((err = ReadHandleCoreIdent(arraylist, g_ArrayListType, (void **)&array))
	    != HandleError_None)
	{
		return ctx->ThrowNativeError("Invalid ArrayList Handle %x (error %d)", arraylist, err);
	}

	auto size = array->size();
	cell_t startidx = 0;
	cell_t count = size;

	if (params[0] > 2)
	{
		startidx = params[3];
		count = params[4];

		if (startidx < 0 || startidx > ((cell_t)size-1))
		{
			return ctx->ThrowNativeError("startidx (%d) must be >=0 and less than the ArrayList size (%d)", startidx, size);
		}

		if (count < 1)
		{
			return ctx->ThrowNativeError("count must be 1 or greater (given %d)", count);
		}

		count = Zmin(count, (cell_t)size-startidx);
	}

	std::vector<Point> data((size_t)count);

	for (int i = 0; i < count; i++)
	{
		cell_t *blk = array->at(startidx+i);
		auto& p = data.at(i);
		memcpy(&p.pos, &blk[offset], sizeof(p.pos));
		// p.idx = i;
		// p.pos[0] = sp_ctof(blk[offset+0]);
		// p.pos[1] = sp_ctof(blk[offset+1]);
		// p.pos[2] = sp_ctof(blk[offset+2]);
	}

	void *object = rust_ClosestPos_Create(data.data(), data.size());

	return g_pHandleSys->CreateHandle(g_ClosestPosType,
		object,
		ctx->GetIdentity(),
		myself->GetIdentity(),
		NULL);
}

static cell_t N_ClosestPos_Find(IPluginContext* ctx, const cell_t* params)
{
	void *object;
	HandleError err;
	HandleSecurity sec(ctx->GetIdentity(), myself->GetIdentity());

	if ((err = handlesys->ReadHandle(params[1], g_ClosestPosType, &sec, (void **)&object))
		!= HandleError_None)
	{
		return ctx->ThrowNativeError("Invalid Handle %x (error: %d)", params[1], err);
	}

	float *query_pt;
	ctx->LocalToPhysAddr(params[2], (cell_t **)&query_pt);

	return rust_ClosestPos_Find(object, query_pt);
}

extern const sp_nativeinfo_t ReplayNatives[] = {
	{"ClosestPos.ClosestPos", N_ClosestPos_ClosestPos},
	{"ClosestPos.Find", N_ClosestPos_Find},
	{NULL, NULL}
};
