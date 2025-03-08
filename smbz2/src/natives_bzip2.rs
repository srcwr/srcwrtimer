// SPDX-License-Identifier: GPL-3.0-or-later
// Copyright 2024 rtldg <rtldg@protonmail.com>
// This file is part of srcwrtimer (https://github.com/srcwr/srcwrtimer/)

// The bzip2 crate that wraps the C bzip2 library, does not actually return any of those bzip2 errors.
// Like Sequence and DataMagic stuff. Seems like the closest you'll get is io::ErrorKind::InvalidInput...
// Whatever...

// I finished this up and then checked out the callback function of the original smbz2...
// They pass sInputPath and then sOutputFile which I did not. I was passing path and path, not path and file.
// Damn. Time for more String allocations.

#![allow(non_snake_case)]

use core::ffi::c_char;
use core::ffi::c_void;
use std::io::BufReader;
use std::io::BufWriter;
use std::io::Write;
use std::num::NonZeroU32;
use std::ptr::NonNull;

// generated by build.rs from bzip2.inc
include!(concat!(env!("OUT_DIR"), "/BZIP2_DEFINES.rs"));

#[derive(Debug)]
pub struct Bz2Info {
	infile:           String,
	outfile:          String,
	infilefull:       String,
	outfilefull:      String,
	forward:          NonNull<c_void>,
	data:             i32,
	compressionLevel: Option<i32>,
	error:            BZ_Error,
}
unsafe impl Send for Bz2Info {} // so we can store the pointers...

fn worker_internal(info: &Bz2Info) -> BZ_Error {
	let Ok(infilefull) = std::fs::File::open(&info.infilefull) else {
		return BZ_Error::BZ_IO_ERROR_INPUT;
	};
	let Ok(outfilefull) = std::fs::File::create(&info.outfilefull) else {
		return BZ_Error::BZ_IO_ERROR_OUTPUT;
	};
	let inbuf = BufReader::new(infilefull);
	let mut outbuf: BufWriter<std::fs::File> = BufWriter::new(outfilefull);

	let ioresult = if let Some(compressionLevel) = info.compressionLevel {
		let compressionLevel = bzip2::Compression::new(compressionLevel as u32);
		let mut compressor = bzip2::read::BzEncoder::new(inbuf, compressionLevel);
		std::io::copy(&mut compressor, &mut outbuf)
	} else {
		let mut decompressor = bzip2::bufread::MultiBzDecoder::new(inbuf);
		std::io::copy(&mut decompressor, &mut outbuf)
	};

	match ioresult {
		Ok(_) => match outbuf.flush() {
			Ok(_) => BZ_Error::BZ_OK,
			_ => BZ_Error::BZ_IO_ERROR,
		},
		_ => BZ_Error::BZ_IO_ERROR,
	}
}

fn worker_thread(mut info: Box<Bz2Info>) {
	info.error = worker_internal(&info);
	unsafe {
		extshared::cpp_add_frame_action(do_callback, Box::into_raw(info) as *mut c_void);
	}
}

extern "C" fn do_callback(info: *mut c_void) {
	unsafe {
		let mut info = Box::from_raw(info as *mut Bz2Info);
		let fw = info.forward;
		info.infile.push('\0');
		info.outfile.push('\0');
		extshared::cpp_forward_push_cell(fw, info.error as i32);
		extshared::cpp_forward_push_string(fw, info.infile.as_ptr()); // path
		extshared::cpp_forward_push_string(fw, info.outfile.as_ptr()); // file
		extshared::cpp_forward_push_cell(fw, info.data);
		extshared::cpp_forward_execute(fw, &mut 0);
		extshared::cpp_forward_release(fw);
	}
}

#[unsafe(no_mangle)]
pub extern "C" fn rust_BZ2_XompressFile(
	infile: *const c_char,
	outfile: *const c_char,
	infilefull: *const c_char,
	outfilefull: *const c_char,
	compressionLevel: i32,
	forward: NonNull<c_void>,
	data: i32,
) -> Option<NonZeroU32> {
	let infile = extshared::strxx(infile, false, 0)?;
	let outfile = extshared::strxx(outfile, false, 0)?;
	let infilefull = extshared::strxx(infilefull, false, 0)?;
	let outfilefull = extshared::strxx(outfilefull, false, 0)?;

	let info = Box::new(Bz2Info {
		infile: infile.to_owned(),
		outfile: outfile.to_owned(),
		infilefull: infilefull.to_owned(),
		outfilefull: outfilefull.to_owned(),
		forward,
		data,
		compressionLevel: if compressionLevel < 0 { None } else { Some(compressionLevel) },
		error: BZ_Error::BZ_OK,
	});

	let _ = std::thread::spawn(move || worker_thread(info));

	NonZeroU32::new(1)
}
