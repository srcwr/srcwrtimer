// SPDX-License-Identifier: GPL-3.0-or-later
// Copyright 2022 rtldg <rtldg@protonmail.com>
// This file is part of srcwrtimer (https://github.com/srcwr/srcwrtimer/)

use std::io::Read;
use std::io::Seek;
use std::io::SeekFrom;
use std::io::Write;

/// cbindgen:no-export
#[repr(transparent)]
pub struct IFileObject {
	pub vtable: usize
}

unsafe extern "C" {
	pub fn IFileObject_Read(file: *const IFileObject, pOut: *mut u8, size: i32) -> usize;
	pub fn IFileObject_Write(file: *const IFileObject, pData: *const u8, size: i32) -> usize;
	pub fn IFileObject_Flush(file: *const IFileObject) -> bool;
	pub fn IFileObject_Seek(file: *const IFileObject, pos: i32, seek_type: i32) -> bool;
	pub fn IFileObject_Tell(file: *const IFileObject) -> usize;
}

impl Read for IFileObject {
	fn read(&mut self, buf: &mut [u8]) -> std::io::Result<usize> {
		// TODO: i32::try_from()
		Ok(unsafe { IFileObject_Read(self, buf.as_mut_ptr(), buf.len() as i32) })
	}
}

impl Write for IFileObject {
	fn write(&mut self, buf: &[u8]) -> std::io::Result<usize> {
		// TODO: i32::try_from()
		Ok(unsafe { IFileObject_Write(self, buf.as_ptr(), buf.len() as i32) })
	}
	fn flush(&mut self) -> std::io::Result<()> {
		if unsafe { IFileObject_Flush(self) } {
			Ok(())
		} else {
			Err(std::io::Error::from(std::io::ErrorKind::Other))
		}
	}
}

// TODO: I don't like this...
impl Seek for IFileObject {
	fn seek(&mut self, pos: SeekFrom) -> std::io::Result<u64> {
		let (asdf, seek_type) = match pos {
			// #define SEEK_SET 0
			// #define SEEK_CUR 1
			// #define SEEK_END 2
			SeekFrom::Start(x) => (x as i32, 0),
			SeekFrom::Current(x) => (x as i32, 1),
			SeekFrom::End(x) => (x as i32, 2),
		};
		if unsafe { IFileObject_Seek(self, asdf, seek_type) } {
			Ok(unsafe { IFileObject_Tell(self) } as u64)
		} else {
			Err(std::io::Error::new(std::io::ErrorKind::Other, "fuck!"))
		}
	}

	fn stream_position(&mut self) -> std::io::Result<u64> {
		Ok(unsafe { IFileObject_Tell(self) } as u64)
	}
}
