// SPDX-License-Identifier: GPL-3.0-or-later
// Copyright 2024 rtldg <rtldg@protonmail.com>
// This file is part of srcwrtimer (https://github.com/srcwr/srcwrtimer/)

#![allow(non_snake_case)]

use std::io::Write;
use std::num::NonZeroU32;

#[unsafe(no_mangle)]
pub extern "C" fn rust_Sample_GetWindowsInfo(
	outbuf: *mut u8,
	outbuflen: usize,
) -> Option<NonZeroU32> {
	let mut buf = unsafe { core::slice::from_raw_parts_mut(outbuf, outbuflen) };
	#[cfg(windows)]
	{
		use windows::Win32::Foundation::NTSTATUS;
		use windows::Win32::System::SystemInformation::OSVERSIONINFOEXW;
		windows_targets::link!("ntdll.dll" "system" fn RtlGetVersion(info: &mut OSVERSIONINFOEXW) -> NTSTATUS);

		let mut info = OSVERSIONINFOEXW::default();
		info.dwOSVersionInfoSize = size_of_val(&info) as u32;
		unsafe {
			let _status = RtlGetVersion(&mut info);
			let _ = write!(
				buf,
				"major={} minor={} build={} platid={} spmajor={} spminor={} suite={} ptype={}",
				info.dwMajorVersion,
				info.dwMinorVersion,
				info.dwBuildNumber,
				info.dwPlatformId,
				info.wServicePackMajor,
				info.wServicePackMinor,
				info.wSuiteMask,
				info.wProductType
			);
		}
	}
	#[cfg(not(windows))]
	{
		let _ = write!(buf, "linux :(");
	}
	NonZeroU32::new(1)
}
