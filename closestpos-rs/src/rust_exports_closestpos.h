// SPDX-License-Identifier: GPL-3.0-or-later
// Copyright 2024 rtldg <rtldg@protonmail.com>
// This file is part of srcwrtimer (https://github.com/srcwr/srcwrtimer/)

#pragma once


struct Point
{
	float pos[3];
};


extern "C" {

void rust_handle_destroy_ClosestPos(void* object);
bool rust_handle_size_ClosestPos(void* object, unsigned int* size);

void* rust_ClosestPos_Create(Point* points, unsigned count);
unsigned rust_ClosestPos_Find(void* object, float* point);

}
