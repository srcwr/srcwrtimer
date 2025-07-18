// SPDX-License-Identifier: GPL-3.0-or-later
// Copyright 2022 rtldg <rtldg@protonmail.com>
// This file is part of srcwrtimer (https://github.com/srcwr/srcwrtimer/)

use core::ffi::c_char;

// TODO: Rename?
#[repr(C)]
#[derive(Debug)]
pub struct ICellArray {
	pub vtable:    usize,
	pub data:      *const c_char,
	pub blocksize: usize,
	pub allocsize: usize,
	pub size:      usize,
}

unsafe extern "C" {
	// TODO: We could look into using a cpp crate to call these or perhaps do some vtable shenanigans...
	pub fn ICellArray_resize(cellarray: *mut ICellArray, count: usize) -> bool;
	pub fn ICellArray_push(cellarray: *mut ICellArray) -> *mut c_char;
	pub fn ICellArray_at(cellarray: *mut ICellArray, index: usize) -> *mut c_char;
	pub fn ICellArray_PushString(cellarray: *mut ICellArray, s: *const u8, len: usize) -> isize;
}

impl ICellArray {
	pub fn push_string(&mut self, s: &str) -> Option<usize> {
		let idx = unsafe { ICellArray_PushString(self, s.as_ptr(), s.len()) };
		if idx < 0 { None } else { Some(idx as usize) }
	}
}
