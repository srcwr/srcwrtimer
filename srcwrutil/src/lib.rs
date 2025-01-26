// SPDX-License-Identifier: GPL-3.0-or-later
// Copyright 2022-2024 rtldg <rtldg@protonmail.com>
// This file is part of srcwrtimer (https://github.com/srcwr/srcwrtimer/)

mod natives_util;
mod string_block;

extshared::smext_conf_boilerplate_extension_info!();

#[unsafe(no_mangle)]
pub fn rust_sdk_on_load(_late: bool) -> Result<(), Box<dyn std::error::Error>> {
	Ok(())
}

#[unsafe(no_mangle)]
pub extern "C" fn rust_sdk_on_unload() {}

#[unsafe(no_mangle)]
pub extern "C" fn rust_sdk_on_all_loaded() {}

#[unsafe(no_mangle)]
pub extern "C" fn rust_on_core_map_start(
	_edict_list: *mut core::ffi::c_void,
	_edict_count: i32,
	_client_max: i32,
) {
}

#[unsafe(no_mangle)]
pub extern "C" fn rust_on_core_map_end() {}
