// SPDX-License-Identifier: GPL-3.0-or-later
// Copyright 2022-2024 rtldg <rtldg@protonmail.com>
// This file is part of srcwrtimer (https://github.com/srcwr/srcwrtimer/)

use std::io::Read;
use std::io::Seek;
use std::io::SeekFrom;
use std::io::Write;

#[repr(transparent)]
pub struct valvefs {
	pub vtable: usize,
}

extern "C" {
	pub fn valvefs_Read(file: *const valvefs, pOut: *mut u8, size: i32) -> usize;
	pub fn valvefs_Write(file: *const valvefs, pData: *const u8, size: i32) -> usize;
	pub fn valvefs_Flush(file: *const valvefs);
	pub fn valvefs_Seek(file: *const valvefs, pos: i32, seek_type: i32);
	pub fn valvefs_Tell(file: *const valvefs) -> usize;
}

impl Read for valvefs {
	fn read(&mut self, buf: &mut [u8]) -> std::io::Result<usize> {
		Ok(unsafe { valvefs_Read(self, buf.as_mut_ptr(), buf.len() as i32) })
	}
}

impl Write for valvefs {
	fn write(&mut self, buf: &[u8]) -> std::io::Result<usize> {
		Ok(unsafe { valvefs_Write(self, buf.as_ptr(), buf.len() as i32) })
	}
	fn flush(&mut self) -> std::io::Result<()> {
		unsafe { valvefs_Flush(self) };
		Ok(())
	}
}

// TODO: I don't like this...
impl Seek for valvefs {
	fn seek(&mut self, pos: SeekFrom) -> std::io::Result<u64> {
		let (asdf, seek_type) = match pos {
			// #define SEEK_SET 0
			// #define SEEK_CUR 1
			// #define SEEK_END 2
			SeekFrom::Start(x) => (x as i32, 0),
			SeekFrom::Current(x) => (x as i32, 1),
			SeekFrom::End(x) => (x as i32, 2),
		};
		unsafe {
			valvefs_Seek(self, asdf, seek_type);
			valvefs_Tell(self) as u64
		}
	}

	fn stream_position(&mut self) -> std::io::Result<u64> {
		Ok(unsafe { valvefs_Tell(self) } as u64)
	}
}
