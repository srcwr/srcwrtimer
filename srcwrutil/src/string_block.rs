// SPDX-License-Identifier: WTFPL
// Copyright 2025 rtldg <rtldg@protonmail.com>
// This file is part of srcwrtimer (https://github.com/srcwr/srcwrtimer/)

#![allow(dead_code)]

pub struct StringBlock {
	// The strings are internally stored as "\x00<string>\x00<string>\x00".
	data:             Vec<u8>,
	// The byte-offset into data to where each string starts.
	starting_indexes: Vec<u32>,
}

impl StringBlock {
	pub fn new() -> StringBlock {
		StringBlock {
			// start the buffer with a '\0' so finding a string is easier...
			// we can search for "\x00<string>\x00" (with `.find_delimited_str()`)
			data:             vec![0],
			starting_indexes: vec![],
		}
	}
	/// Returns the number (accessible) strings in the StringBlock.
	pub fn len(&self) -> usize {
		self.starting_indexes.len()
	}
	/// Clears the internal backing structures but does not free memory.
	pub fn clear(&mut self) {
		// first byte is a '\0' so truncate it to that
		self.data.truncate(1);

		self.starting_indexes.clear();
	}
	/// Used to prevent the `push<S: AsRef<str>>()` genericization from duplicating a bunch of code.
	fn push_str(&mut self, s: &str) -> usize {
		let start = self.data.len();
		self.data.extend_from_slice(s.as_bytes());
		self.data.push(b'\0');
		self.starting_indexes.push(start as u32);
		self.starting_indexes.len() - 1
	}
	/// Append a string to the string block and return the its index.
	pub fn push<S: AsRef<str>>(&mut self, s: S) -> usize {
		self.push_str(s.as_ref())
	}
	/*
	pub fn lowercase_all_strings(&mut self) {
		unsafe {
			let s = self.data.as_mut_slice();
			let s = std::str::from_utf8_unchecked_mut(s);
			s.make_ascii_lowercase();
		}
	}
	*/
	pub fn get_str_ptr(&self, idx: usize) -> Option<*const i8> {
		let idx = self.starting_indexes.get(idx)?;
		unsafe { Some((self.data.as_ptr() as *const i8).add(*idx as usize)) }
	}
	pub fn get_str(&self, idx: usize) -> Option<&str> {
		let p = self.get_str_ptr(idx)?;
		Some(unsafe { std::str::from_utf8_unchecked(std::ffi::CStr::from_ptr(p).to_bytes()) })
	}
	/// `target` should be a string like "\x00<string>\x00"
	pub fn find_delimited_str(&self, target: &str) -> Option<usize> {
		let s = unsafe { std::str::from_utf8_unchecked(&self.data) };
		let byte_idx = (s.find(target)? + 1) as u32; // +1 to skip the \x00
		self.starting_indexes
			.iter()
			.position(|idx| *idx == byte_idx)
	}
	pub fn find_str(&self, target: &str) -> Option<usize> {
		for (i, s) in self.iter_str().enumerate() {
			if s == target {
				return Some(i);
			}
		}
		None
	}
	pub fn find_c_str(&self, target: *const i8) -> Option<usize> {
		for (i, s) in self.iter_ptr().enumerate() {
			if unsafe { libc::strcmp(s, target) } == 0 {
				return Some(i);
			}
		}
		None
	}
	pub fn iter_str(&self) -> StringBlockStrIter {
		StringBlockStrIter {
			sb:           self,
			indexes_iter: self.starting_indexes.iter(),
		}
	}
	pub fn iter_ptr(&self) -> StringBlockPtrIter {
		StringBlockPtrIter {
			sb:           self,
			indexes_iter: self.starting_indexes.iter(),
		}
	}
}

pub struct StringBlockStrIter<'a> {
	sb:           &'a StringBlock,
	indexes_iter: core::slice::Iter<'a, u32>,
}

pub struct StringBlockPtrIter<'a> {
	sb:           &'a StringBlock,
	indexes_iter: core::slice::Iter<'a, u32>,
}

impl<'a> Iterator for StringBlockStrIter<'a> {
	type Item = &'a str;
	fn next(&mut self) -> Option<Self::Item> {
		let idx = self.indexes_iter.next()?;
		unsafe {
			let p = (self.sb.data.as_ptr() as *const i8).add(*idx as usize);
			let cstr = std::ffi::CStr::from_ptr(p);
			let s = std::str::from_utf8_unchecked(cstr.to_bytes());
			Some(s)
		}
	}
}

impl<'a> Iterator for StringBlockPtrIter<'a> {
	type Item = *const i8;
	fn next(&mut self) -> Option<Self::Item> {
		let idx = self.indexes_iter.next()?;
		unsafe {
			let p = (self.sb.data.as_ptr() as *const i8).add(*idx as usize);
			Some(p)
		}
	}
}
