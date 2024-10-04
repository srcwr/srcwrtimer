// SPDX-License-Identifier: GPL-3.0-or-later
// Copyright 2022-2024 rtldg <rtldg@protonmail.com>
// This file is part of srcwrtimer (https://github.com/srcwr/srcwrtimer/)

#![allow(unused_variables)]
#![allow(non_snake_case)]

use core::ffi::c_char;
use std::num::NonZeroU32;

use extshared::IFileObject::IFileObject;
use sha1::Digest;
use sha1::Sha1;

/*
fn dirwalker() {
  let bytes = include_bytes!("dummy.nav");
  let _ = std::fs::hard_link("dummy.nav", "new.nav");
}
*/

#[unsafe(no_mangle)]
pub extern "C" fn rust_handle_destroy_SmolStringList(object: u32) {
	//
}
#[unsafe(no_mangle)]
pub extern "C" fn rust_handle_size_SmolStringList(object: u32, size: &mut u32) -> bool {
	false
}

#[unsafe(no_mangle)]
pub extern "C" fn rust_SRCWRUTIL_GetSHA1_File(
	fileobject: &mut IFileObject,
	buffer: *mut u8,
) -> Option<NonZeroU32> {
	let mut hasher = Sha1::new();
	if let Ok(bytes) = std::io::copy(fileobject, &mut hasher) {
		let result = hasher.finalize();
		let buffer = unsafe { std::slice::from_raw_parts_mut(buffer, 40) };
		hex::encode_to_slice(result, buffer).unwrap();
		NonZeroU32::new(bytes as u32)
	} else {
		None
	}
}

#[unsafe(no_mangle)]
pub extern "C" fn rust_SRCWRUTIL_GetSHA1_FilePath(
	filename: *const c_char,
	buffer: *mut u8,
) -> Option<NonZeroU32> {
	let filename = extshared::strxx(filename, false, 0)?;
	let mut file = std::fs::File::open(filename).ok()?;
	let mut hasher = Sha1::new();
	if let Ok(bytes) = std::io::copy(&mut file, &mut hasher) {
		let result = hasher.finalize();
		let buffer = unsafe { std::slice::from_raw_parts_mut(buffer, 40) };
		hex::encode_to_slice(result, buffer).unwrap();
		NonZeroU32::new(bytes as u32)
	} else {
		None
	}
}
