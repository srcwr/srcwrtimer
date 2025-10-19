// SPDX-License-Identifier: GPL-3.0-or-later
// Copyright 2022-2024 rtldg <rtldg@protonmail.com>
// This file is part of srcwrtimer (https://github.com/srcwr/srcwrtimer/)

#![allow(non_snake_case)]

use core::ffi::c_void;
use std::ffi::CStr;
use std::ffi::c_char;
use std::io::Write;
use std::ptr::NonNull;

pub mod ICellArray;
pub mod IFileObject;
pub mod export_sourcemod_extension_symbols;
pub mod smext_conf_boilerplate;

/*
#define SM_PARAM_COPYBACK       (1<<0)      /**< Copy an array/reference back after call */

#define SM_PARAM_STRING_UTF8    (1<<0)      /**< String should be UTF-8 handled */
#define SM_PARAM_STRING_COPY    (1<<1)      /**< String should be copied into the plugin */
#define SM_PARAM_STRING_BINARY  (1<<2)      /**< Treat the string as a binary string */
*/
pub const COPYBACK: i32 = 1 << 0;

pub const STR_UTF8: i32 = 1 << 0;
pub const STR_COPY: i32 = 1 << 1;
pub const STR_BINARY: i32 = 1 << 2;

#[allow(non_camel_case_types)]
#[repr(C)]
pub enum PathType {
	Path_None = 0,
	Path_Game,
	Path_SM,
	Path_SM_Rel,
}

pub type GameFrameHookFunc = unsafe extern "C" fn(simulating: bool);
pub type FrameActionFunc = unsafe extern "C" fn(data: *mut c_void);
unsafe extern "C" {
	pub fn cpp_free_handle(handle: u32);
	pub fn cpp_add_frame_action(func: FrameActionFunc, data: *mut c_void);
	pub fn cpp_add_game_frame_hook(hook: GameFrameHookFunc);
	pub fn cpp_remove_game_frame_hook(hook: GameFrameHookFunc);

	pub fn cpp_forward_release(forward: NonNull<c_void>);
	pub fn cpp_forward_function_count(forward: NonNull<c_void>) -> u32;
	pub fn cpp_forward_push_cell(forward: NonNull<c_void>, cell: i32) -> i32;
	pub fn cpp_forward_push_string(forward: NonNull<c_void>, s: *const u8) -> i32;
	pub fn cpp_forward_push_string_ex(
		forward: NonNull<c_void>,
		s: *mut u8,
		len: usize,
		str_flags: i32,
		copy_flags: i32,
	) -> i32;
	// TODO: PushArray, PushCellByRef
	pub fn cpp_forward_execute(forward: NonNull<c_void>, result: &mut i32) -> i32;

	pub fn cpp_report_error(ctx: NonNull<c_void>, err: *const u8);

	pub unsafe fn cpp_extension_log_message(msg: *const u8);
	pub unsafe fn cpp_extension_log_error(msg: *const u8);

	pub fn cpp_local_to_phys_addr(ctx: NonNull<c_void>, addr: i32) -> *mut u8;
	pub fn cpp_string_to_local_utf8(ctx: NonNull<c_void>, addr: i32, maxbytes: usize, s: *const u8) -> usize;

	pub fn cpp_build_path(buf: *mut u8, bufsize: usize, friendly_path: *const u8, ty: PathType);
}

pub fn build_path(friendly_path: *const u8, ty: PathType) -> String {
	let mut buf: [u8; 260] = [0; 260];
	unsafe {
		cpp_build_path(buf.as_mut_ptr(), buf.len(), friendly_path, ty);
	}
	CStr::from_bytes_until_nul(&buf).unwrap().to_str().unwrap().to_owned()
}

// I don't want to allocate a String/CString to print an error...
#[allow(clippy::not_unsafe_ptr_arg_deref)]
pub fn report_error(ctx: NonNull<c_void>, err: &dyn std::error::Error) {
	const BUFSIZE: usize = 1024; // 1024 comes from the inner buffer used by ReportErrorVA
	// Note to self: Trying to use unitialized memory for this is a hassle.
	let mut buf = [0u8; BUFSIZE];
	let _ = write!(&mut buf[..BUFSIZE - 1], "{}", err); // -1 to leave at least the last byte as '\0'
	unsafe { cpp_report_error(ctx, buf.as_ptr()) }
}

/// Turn C-string pointer into a &str.
/// If `end < 1` then we calculate the length with `strlen()`.`
/// Returns `None` if an empty string is passed.
#[allow(clippy::not_unsafe_ptr_arg_deref)]
pub fn strxx<'a>(s: *const c_char, allow_invalid_utf8: bool, end: i32) -> Option<&'a str> {
	unsafe {
		// LLVM optimizes out this strlen thing or something...
		unsafe extern "C" {
			fn strlen(s: *const c_char) -> usize;
		}
		let len = if end < 1 { strlen(s) } else { end as usize };

		if len == 0 {
			return None;
		}

		let bytes = std::slice::from_raw_parts(s as *const u8, len);

		if allow_invalid_utf8 {
			Some(std::str::from_utf8_unchecked(bytes))
		} else {
			simdutf8::basic::from_utf8(bytes).ok()
		}
	}
}

#[allow(unused_variables)]
#[allow(clippy::not_unsafe_ptr_arg_deref)]
pub fn write_to_sp_buf(
	s: &[u8],
	ctx: Option<NonNull<c_void>>,
	buffer: *mut u8,
	local_addr: i32,
	maxlength: usize,
	flags: i32,
) -> Option<core::num::NonZeroUsize> {
	if false {
		// TODO: use flags to do extshared::cpp_string_to_local_utf8(...) stuff here?
	}

	let buffer = unsafe { std::slice::from_raw_parts_mut(buffer, maxlength) };

	let sz = core::cmp::min(buffer.len() - 1, s.len());
	buffer[..sz].clone_from_slice(&s[..sz]);
	buffer[sz] = b'\0';
	unsafe { Some(core::num::NonZeroUsize::new_unchecked(sz)) }
}
