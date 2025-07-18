// SPDX-License-Identifier: GPL-3.0-or-later
// Copyright 2022-2025 rtldg <rtldg@protonmail.com>
// This file is part of srcwrtimer (https://github.com/srcwr/srcwrtimer/)

use core::ffi::c_void;

unsafe extern "C" {
	#[cfg(target_os = "linux")]
	pub fn GetSMExtAPIxxx() -> *const c_void;
	#[cfg(target_os = "windows")]
	pub fn GetSMExtAPI() -> *const c_void;

	#[cfg(target_os = "linux")]
	pub fn CreateInterface_MMSxxx(mvi: *const c_void, mli: *const c_void) -> *const c_void;
}

/// This is our hack to "re-export" GetSMExtAPI from smsdk_ext.cpp...
/// Relevant rust issue: https://github.com/rust-lang/rfcs/issues/2771
/// On Linux, I make extshared_build_helper/src/lib.rs define `GetSMExtAPI` to `GetSMExtAPIxxx`
/// so I don't have to modify smsdk_ext.cpp...
#[cfg(target_os = "linux")]
#[unsafe(no_mangle)]
pub unsafe extern "C" fn GetSMExtAPI() -> *const c_void {
	unsafe { GetSMExtAPIxxx() }
}

#[cfg(target_os = "linux")]
#[unsafe(no_mangle)]
pub unsafe extern "C" fn CreateInterface_MMS(mvi: *const c_void, mli: *const c_void) -> *const c_void {
	unsafe { CreateInterface_MMSxxx(mvi, mli) }
}

/// We have to call this so the symbol won't be optimized out...
pub fn doit() {
	let _ = unsafe { GetSMExtAPI() };
}

/*
pub fn hello() {
  println!(
	">>> hello from {}! GetSMExtAPI() = {:?}",
	env!("CARGO_PKG_NAME"),
	unsafe { GetSMExtAPI() }
  );
}
*/
