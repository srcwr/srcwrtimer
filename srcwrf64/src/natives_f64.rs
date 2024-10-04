// SPDX-License-Identifier: GPL-3.0-or-later
// Copyright 2022 rtldg <rtldg@protonmail.com>
// This file is part of srcwrtimer (https://github.com/srcwr/srcwrtimer/)

#![allow(non_snake_case)]

use core::ffi::c_char;
use core::ffi::c_void;
use core::ffi::CStr;

//////////////////////////////////////////////////////////////////////////////////////////////////////////////

#[no_mangle]
pub extern "C" fn rust_F64_FromString(s: *const c_char) -> f64 {
  let mut buffer = ryu::Buffer::new();
  let printed = buffer.format_finite(1.234);
  0
}

#[no_mangle]
pub extern "C" fn rust_F64_ToString(s: *const c_char) -> f64 {
  let mut buffer = ryu::Buffer::new();
  let printed = buffer.format_finite(1.234);
  0
}

//////////////////////////////////////////////////////////////////////////////////////////////////////////////
