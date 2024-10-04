// SPDX-License-Identifier: GPL-3.0-or-later
// Copyright 2022-2024 rtldg <rtldg@protonmail.com>
// This file is part of srcwrtimer (https://github.com/srcwr/srcwrtimer/)

// TODO: It would be cool if we could make these macros only define these functions if the file does not already define them...
//       That might be proc-macro territory though which sucks...

#[macro_export]
macro_rules! smext_conf_boilerplate_extension_info {
	() => {
		use std::io::Write;

		#[no_mangle]
		pub extern "C" fn rust_conf_name() -> *const u8 {
			concat!(env!("CARGO_PKG_NAME"), "\0").as_ptr()
		}
		#[no_mangle]
		pub extern "C" fn rust_conf_description() -> *const u8 {
			concat!(env!("CARGO_PKG_DESCRIPTION"), "\0").as_ptr()
		}
		#[no_mangle]
		pub extern "C" fn rust_conf_version() -> *const u8 {
			concat!(env!("CARGO_PKG_VERSION"), "\0").as_ptr()
		}
		#[no_mangle]
		pub extern "C" fn rust_conf_author() -> *const u8 {
			concat!(env!("CARGO_PKG_AUTHORS"), "\0").as_ptr()
		}
		#[no_mangle]
		pub extern "C" fn rust_conf_url() -> *const u8 {
			concat!(
				env!("CARGO_PKG_HOMEPAGE"),
				"/tree/master/",
				env!("CARGO_PKG_NAME"),
				"\0"
			)
			.as_ptr()
		}
		#[no_mangle]
		pub extern "C" fn rust_conf_logtag() -> *const u8 {
			concat!(env!("CARGO_PKG_NAME"), "\0").as_ptr()
		}
		#[no_mangle]
		pub extern "C" fn rust_conf_license() -> *const u8 {
			concat!(env!("CARGO_PKG_LICENSE"), "\0").as_ptr()
		}

		#[no_mangle]
		#[allow(clippy::not_unsafe_ptr_arg_deref)]
		pub extern "C" fn rust_sdk_on_load_wrapper(
			error: *mut u8,
			maxlength: usize,
			late: bool,
		) -> bool {
			extshared::export_GetSMExtAPI::doit();
			match rust_sdk_on_load(late) {
				Ok(_) => true,
				Err(e) => {
					let errorbuffer = unsafe { std::slice::from_raw_parts_mut(error, maxlength) };
					let _result = write!(&mut errorbuffer[..maxlength - 1], "{}\0", e); // yes, [..maxlength-1]...
					errorbuffer[maxlength - 1] = b'\0'; // for real-zies, NUL-terminate last element just in case.
					false
				},
			}
		}
	};
}

#[macro_export]
macro_rules! smext_conf_boilerplate_load_funcs {
	() => {
		// perhaps an anyhow::Result<()> would be better...
		#[no_mangle]
		pub fn rust_sdk_on_load(_late: bool) -> Result<(), Box<dyn std::error::Error>> {
			Ok(())
		}

		#[no_mangle]
		pub extern "C" fn rust_sdk_on_unload() {}

		#[no_mangle]
		pub extern "C" fn rust_sdk_on_all_loaded() {}

		#[no_mangle]
		pub extern "C" fn rust_on_core_map_start(
			_edict_list: *mut core::ffi::c_void,
			_edict_count: i32,
			_client_max: i32,
		) {
		}

		#[no_mangle]
		pub extern "C" fn rust_on_core_map_end() {}
	};
}
