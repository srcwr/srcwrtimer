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
The mean byte-length of mapnames on fastdl.me is around 15.48 to 15.56, depending on the list used.
Using the snippet below, we can also see that it starts falling off after 20 bytes:
	import numpy as np
	lines = open("processed/main.fastdl.me/69.html.txt", "r", encoding="utf8").read().strip().splitlines()
	lengths = np.array([len(line.strip()) for line in lines])
	unique, counts = np.unique(lengths, return_counts=True)
	print(np.asarray((unique, counts)).T)
Also the longest map name is 57 bytes.

How we use the mapchooser lists:
	list.Length
	list.GetString(idx, s, sizeof(s))
	list.FindString(s)
	list.Clear()
	list.PushString(s)
	list.SwapAt(i, --length) & list.Resize(length) # a swap_remove() for removing excludes
	list.SortCustom(SlowSortThatSkipsFolders)
	ReadMapList(list, serial, "default", MAPLIST_FLAG_CLEARARRAY) # sourcemod native

A crate I can use so I don't have to write one myself that fits most of these:
	https://docs.rs/compact_strings/latest/compact_strings/struct.CompactStrings.html
This one exists too but doesn't provide removing strings:
	https://docs.rs/packed_str/latest/packed_str/struct.PackedStr.html
We could also look into an arena and then store the string slices in a Vec:
	https://docs.rs/bumpalo/latest/bumpalo/struct.Bump.html#method.alloc_str
	https://docs.rs/typed-arena/latest/typed_arena/struct.Arena.html#method.alloc_str
*/

pub struct SmolStringList;

#[unsafe(no_mangle)]
pub extern "C" fn rust_handle_destroy_SmolStringList(object: &mut SmolStringList) {
	//
}
#[unsafe(no_mangle)]
pub extern "C" fn rust_handle_size_SmolStringList(object: &mut SmolStringList, size: &mut u32) -> bool {
	false
}

#[unsafe(no_mangle)]
pub extern "C" fn rust_SRCWRUTIL_GetSHA1_File(fileobject: &mut IFileObject, buffer: *mut c_char) -> Option<NonZeroU32> {
	let mut hasher = Sha1::new();
	if let Ok(bytes) = std::io::copy(fileobject, &mut hasher) {
		let result = hasher.finalize();
		let buffer = unsafe { std::slice::from_raw_parts_mut(buffer as *mut _, 40) };
		hex::encode_to_slice(result, buffer).unwrap();
		NonZeroU32::new(bytes as u32)
	} else {
		None
	}
}

#[unsafe(no_mangle)]
pub extern "C" fn rust_SRCWRUTIL_GetSHA1_FilePath(filename: *const c_char, buffer: *mut c_char) -> Option<NonZeroU32> {
	let filename = extshared::strxx(filename, false, 0)?;
	let mut file = std::fs::File::open(filename).ok()?;
	let mut hasher = Sha1::new();
	if let Ok(bytes) = std::io::copy(&mut file, &mut hasher) {
		let result = hasher.finalize();
		let buffer = unsafe { std::slice::from_raw_parts_mut(buffer as *mut _, 40) };
		hex::encode_to_slice(result, buffer).unwrap();
		NonZeroU32::new(bytes as u32)
	} else {
		None
	}
}
