// SPDX-License-Identifier: GPL-3.0-or-later
// Copyright 2022-2023 rtldg <rtldg@protonmail.com>
// This file is part of srcwrtimer (https://github.com/srcwr/srcwrtimer/)

#pragma once

#include <stddef.h>
#include <stdint.h> // make vscode stop complaining about int64_t plz

class IFileObject;

typedef unsigned bigbool;

extern "C" {

void* rust_SRCWRJSON_UnsafePointer(void* object);

void rust_handle_destroy_SRCWRJSON(void* object);
bool rust_handle_size_SRCWRJSON(void* object, unsigned int* size);

void* rust_SRCWRJSON_SRCWRJSON(bool array);
void* rust_SRCWRJSON_Clone(void* object);

void rust_SRCWRJSON_GetObjects(void** objects, size_t count);

bigbool rust_SRCWRJSON_ToFile(void* object, const char* filename, int flags, const char* key, int keylen);
bigbool rust_SRCWRJSON_ToFileHandle(void* object, IFileObject* fileobject, int flags, const char* key, int keylen);

size_t rust_SRCWRJSON_ToString(
	void* ctx,
	void* object,
	char* buffer,
	int local_addr,
	size_t maxlength,
	int flags,
	char* key,
	int keylen);

void* rust_SRCWRJSON_FromFile(const char* filename, int flags);
void* rust_SRCWRJSON_FromFileHandle(IFileObject* fileobject, int flags);
void* rust_SRCWRJSON_FromString(int flags, const char* s, int slen);

bool rust_SRCWRJSON_Has(void* object, int flags, const char* key, int keylen);
int rust_SRCWRJSON_GetType(void* object, int flags, const char* key, int keylen);
bool rust_SRCWRJSON_IsArray(void* object, int flags, const char* key, int keylen);

size_t rust_SRCWRJSON_len(void* object, int flags, const char* key, int keylen);

void* rust_SRCWRJSON_Get(void* object, int flags, const char* key, int keylen);
void* rust_SRCWRJSON_GetIdx(void* object, int flags, int idx);
bigbool rust_SRCWRJSON_Set(void* object, void* other, int flags, const char* key, int keylen);
bigbool rust_SRCWRJSON_SetIdx(void* object, void* other, int flags, int idx);

bigbool rust_SRCWRJSON_SetFromString(void* object, const char* s, int end, int flags, const char* key, int keylen);
bigbool rust_SRCWRJSON_SetFromStringIdx(void* object, const char* s, int end, int flags, int idx);

bigbool rust_SRCWRJSON_Remove(void* object, int flags, const char* key, int keylen);
bigbool rust_SRCWRJSON_RemoveIdx(void* object, int flags, int idx);

int rust_SRCWRJSON_RemoveAllWithSelector(void* object, const char* selector, int flags, const char* key, int keylen);

bigbool rust_SRCWRJSON_Clear(void* object, int flags, const char* key, int keylen);

int rust_SRCWRJSON_ReplaceCell(void* object, int newcell, int flags, const char* key, int keylen);
int rust_SRCWRJSON_ReplaceCellIdx(void* object, int newcell, int flags, int idx);

bigbool rust_SRCWRJSON_SetZss(void* object, int flags, const char* key, void* other, const char* key2, int key2len);

bigbool rust_SRCWRJSON_GetStruct(void* object, unsigned* buf, const char* format, int flags, const char* key, int keylen);
bigbool rust_SRCWRJSON_GetStructIdx(void* object, unsigned* buf, const char* format, int flags, int idx);
bigbool rust_SRCWRJSON_SetStruct(void* object, const unsigned* buf, const char* format, int flags, const char* key, int keylen);
bigbool rust_SRCWRJSON_SetStructIdx(void* object, const unsigned* buf, const char* format, int flags, int idx);

int rust_SRCWRJSON_GetCell(void* object, int flags, const char* key, int keylen);
int rust_SRCWRJSON_GetCellIdx(void* object, int flags, int idx);
bigbool rust_SRCWRJSON_SetCell(void* object, int value, int flags, const char* key, int keylen);
bigbool rust_SRCWRJSON_SetCellIdx(void* object, int value, int flags, int idx);

int rust_SRCWRJSON_GetF32(void* object, int flags, const char* key, int keylen);
int rust_SRCWRJSON_GetF32Idx(void* object, int flags, int idx);
bigbool rust_SRCWRJSON_SetF32(void* object, float value, int flags, const char* key, int keylen);
bigbool rust_SRCWRJSON_SetF32Idx(void* object, float value, int flags, int idx);

bigbool rust_SRCWRJSON_GetBool(void* object, int flags, const char* ke, int keyleny);
bigbool rust_SRCWRJSON_GetBoolIdx(void* object, int flags, int idx);

bigbool rust_SRCWRJSON_SetBool(void* object, bool value, int flags, const char* key, int keylen);
bigbool rust_SRCWRJSON_SetBoolIdx(void* object, bool value, int flags, int idx);
bigbool rust_SRCWRJSON_ToggleBool(void* object, int flags, const char* key, int keylen);
bigbool rust_SRCWRJSON_ToggleBoolIdx(void* object, int flags, int idx);

bigbool rust_SRCWRJSON_GetI64(void* object, int64_t* buffer, int flags, const char* key, int keylen);
bigbool rust_SRCWRJSON_GetI64Idx(void* object, int64_t* buffer, int flags, int idx);
bigbool rust_SRCWRJSON_SetI64(void* object, int64_t buffer, int flags, const char* key, int keylen);
bigbool rust_SRCWRJSON_SetI64Idx(void* object, int64_t buffer, int flags, int idx);

bigbool rust_SRCWRJSON_GetF64(void* object, double* buffer, int flags, const char* key, int keylen);
bigbool rust_SRCWRJSON_GetF64Idx(void* object, double* buffer, int flags, int idx);
bigbool rust_SRCWRJSON_SetF64(void* object, double buffer, int flags, const char* key, int keylen);
bigbool rust_SRCWRJSON_SetF64Idx(void* object, double buffer, int flags, int idx);

bigbool rust_SRCWRJSON_IsNull(void* object, int flags, const char* key, int keylen);
bigbool rust_SRCWRJSON_IsNullIdx(void* object, int flags, int idx);
bigbool rust_SRCWRJSON_SetNull(void* object, int flags, const char* key, int keylen);
bigbool rust_SRCWRJSON_SetNullIdx(void* object, int flags, int idx);

size_t rust_SRCWRJSON_GetString(
	void* ctx,
	void* object,
	char* buffer,
	int local_addr,
	size_t maxlength,
	int flags,
	const char* key,
	int keylen);

size_t rust_SRCWRJSON_GetStringIdx(
	void* ctx,
	void* object,
	char* buffer,
	int local_addr,
	size_t maxlength,
	int flags,
	int idx);

bigbool rust_SRCWRJSON_SetString(void* object, const char* s, int end, int flags, const char* key, int keylen);
bigbool rust_SRCWRJSON_SetStringIdx(void* object, const char* s, int end, int flags, int idx);

bigbool rust_SRCWRJSON_FillKeys(void* object, void* cellarray, int flags, const char* key, int keylen);

}
