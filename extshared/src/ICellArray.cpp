// SPDX-License-Identifier: GPL-3.0-or-later
// Copyright 2022 rtldg <rtldg@protonmail.com>
// This file is part of srcwrtimer (https://github.com/srcwr/srcwrtimer/)

#include <sp_vm_types.h>
#include <sm_platform.h> // PLATFORM_EXTERN_C
#include <ICellArray.h>
#include <string.h>

using namespace SourceMod;

#if 0
class PubCellArray : public ICellArray
{
public:
	char* data_;
	size_t blocksize_;
	size_t allocsize_;
	size_t size_;
}
#endif

PLATFORM_EXTERN_C bool ICellArray_resize(ICellArray* cellarray, size_t count)
{
	return cellarray->resize(count);
}

PLATFORM_EXTERN_C char* ICellArray_push(ICellArray* cellarray)
{
	return (char*)cellarray->push();
}

PLATFORM_EXTERN_C char* ICellArray_at(ICellArray* cellarray, size_t index)
{
	return (char*)cellarray->at(index);
}

PLATFORM_EXTERN_C size_t ICellArray_PushString(ICellArray* cellarray, const char* str, size_t len)
{
	if (len < 1) len = strlen(str);
	if (!len) return -1;
	char* ptr = (char*)cellarray->push();
	if (!ptr) return -1;
	auto maxStringSpace = cellarray->blocksize() * 4 - 1;
	len = maxStringSpace < len ? maxStringSpace : len;
	memcpy(ptr, str, len);
	*(ptr + len) = '\0';
	return cellarray->size() - 1;
}

// TODO: Add that resize&reblocksize stuff from bhoptimer_helper_original
// https://github.com/srcwr/bhoptimer_helper_original/blob/1b942143255e4f6e98a2a0848b93bba0b5da737d/src/replay.rs#L291-L297
/*
	arraylist.m_Size *= arraylist.m_BlockSize;
	arraylist.m_AllocSize *= arraylist.m_BlockSize;
	arraylist.m_BlockSize = 1;
	arraylist.resize(total_cells)?;
	arraylist.m_Size = total_cells;
	arraylist.m_AllocSize = arraylist.m_Size / total_cells;
	arraylist.m_BlockSize = total_cells;
*/
