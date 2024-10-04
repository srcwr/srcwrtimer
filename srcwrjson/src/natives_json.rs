// SPDX-License-Identifier: GPL-3.0-or-later
// Copyright 2022-2024 rtldg <rtldg@protonmail.com>
// This file is part of srcwrtimer (https://github.com/srcwr/srcwrtimer/)

#![allow(non_snake_case)]

// TODO: Throw core::intrinsics::likely() everywhere :innocent:

use core::ffi::c_char;
use core::ffi::c_void;
use core::ffi::CStr;
use core::ptr::NonNull;
use std::fs::File;
use std::io::BufRead;
use std::io::BufReader;
use std::io::Read;
use std::io::Write;
use std::num::NonZeroI32;
use std::num::NonZeroU32;
use std::num::NonZeroUsize;

use extshared::ICellArray::ICellArray;
use extshared::IFileObject::IFileObject;
use extshared::*;
//use jsonpath_lib::SelectorMut;
use serde_json::json;
// use serde_json::value::Index;
use serde_json::value::Value;
use smallvec::SmallVec;

///////////////////////////////////////////////////////////////////////////////////////

// generated by build.rs from json.inc
include!(concat!(env!("OUT_DIR"), "/JSON_DEFINES.rs"));

///////////////////////////////////////////////////////////////////////////////////////

#[derive(Debug)]
#[allow(clippy::upper_case_acronyms)]
pub struct SRCWRJSON {
	v: Value,
}

///////////////////////////////////////////////////////////////////////////////////////

#[unsafe(no_mangle)]
pub extern "C" fn rust_handle_destroy_SRCWRJSON(object: *mut SRCWRJSON) {
	drop(unsafe { Box::from_raw(object) })
}

#[unsafe(no_mangle)]
pub extern "C" fn rust_handle_size_SRCWRJSON(object: &SRCWRJSON, size: &mut u32) -> bool {
	*size = std::mem::size_of::<SRCWRJSON>() as u32 + recursive_size_calc(&object.v) as u32; //stacker::grow(1 * 1024 * 1025, || recursive_size_calc(&obj.v)) as u32;
	true
}

fn recursive_size_calc(value: &Value) -> usize {
	// PLEASE make a recursive json value that blows up the stack IT WOULD BE SO FUNNY
	std::mem::size_of::<SRCWRJSON>()
		+ match value {
			Value::String(s) => s.capacity(),
			Value::Array(v) => v.iter().fold(0, |a, x| a + recursive_size_calc(x)),
			Value::Object(v) => v.iter().fold(0, |a, (_k, x)| a + recursive_size_calc(x)),
			_ => 0,
		}
}

///////////////////////////////////////////////////////////////////////////////////////
// Helper functions...

fn boxval(V: Value) -> NonNull<SRCWRJSON> {
	let boxed = Box::new(SRCWRJSON { v: V });
	unsafe { NonNull::new_unchecked(Box::into_raw(boxed)) }
}

fn Setx_Inner(
	object: &mut Value,
	other: &mut Value,
	flags: i32,
	key: *const c_char,
	keylen: i32,
) -> Option<NonZeroU32> {
	let object = sidx(object, key, flags, true, keylen)?;
	let ret = if 0 != (flags & J_IS_DIFFERENT) && !object.eq(&other) {
		None
	} else {
		NonZeroU32::new(1)
	};
	std::mem::swap(object, other);
	ret
}

fn sidx(
	object: &mut Value,
	s: *const c_char,
	flags: i32,
	create_if_not_exists: bool,
	end: i32,
) -> Option<&mut Value> {
	if s.is_null() {
		return Some(object);
	}
	let key = strxx(s, 0 != (flags & J_UNCHECKED_UTF8_KEY), end)?;

	if 0 != (flags & J_NO_POINTER) || key.as_bytes()[0] != b'/' {
		let m = object.as_object_mut()?;
		if create_if_not_exists {
			return Some(m.entry(key).or_insert(Value::Null));
		} else {
			return m.get_mut(key);
		}
	}

	let create_parents = 0 != (flags & J_CREATE_PARENTS);

	let mut parents: SmallVec<[&str; 8]> =
		key.split('/').skip(1).filter(|&x| !x.is_empty()).collect();
	let base = parents.pop()?;

	let mut scratch = SmallVec::<[u8; 128]>::new();
	let mut current_value = object;

	for p in &parents {
		let k = unescape_pointer3(p, &mut scratch);
		current_value = match current_value {
			Value::Object(m) => {
				if create_parents {
					Some(m.entry(k).or_insert(Value::Null))
				} else {
					m.get_mut(k)
				}
			},
			Value::Array(a) => {
				let arraylen = a.len();
				let idx = as_array_idx(k, arraylen)?;
				if arraylen < idx + 1 {
					if !create_parents {
						return None;
					}
					a.resize(idx + 1, Value::Null);
				}
				Some(&mut a[idx])
			},
			Value::Null => {
				if !create_parents {
					return None;
				}
				if let Some(idx) = as_array_idx(k, 0) {
					*current_value = json!([]);
					let a = current_value.as_array_mut()?;
					a.resize(idx + 1, Value::Null);
					Some(&mut a[idx])
				} else {
					*current_value = json!({});
					Some(
						current_value
							.as_object_mut()
							.unwrap()
							.entry(k)
							.or_insert(Value::Null),
					)
				}
			},
			_ => None,
		}?;
	}

	if let Value::Null = current_value {
		// TODO: Maybe just check for everything non-array & non-object?
		if !create_parents {
			return None;
		}
		*current_value = if let Some(_idx) = as_array_idx(base, 0) {
			json!([])
		} else {
			json!({})
		};
	}

	match current_value {
		Value::Array(x) => {
			let xlen = x.len();
			let idx = as_array_idx(base, xlen)?;
			if idx >= xlen {
				if (idx - xlen) > 0 || !create_if_not_exists {
					return None;
				}
				// x.resize(idx + 1, Value::Null);
				x.push(Value::Null);
			}
			Some(&mut x[idx])
		},
		Value::Object(x) => {
			if create_if_not_exists {
				let abc = x.entry(base).or_insert(Value::Null);
				Some(abc)
			} else {
				x.get_mut(base)
			}
		},
		_ => None,
	}
}

// a single `-`, // I call this the appender
// a single `0`,
// a negative number
// or a digits that don't start with a zero...
fn as_array_idx(mut s: &str, total_len: usize) -> Option<usize> {
	if s == "-" {
		s = "-1";
	}
	if s.starts_with('-') {
		let tmp: isize = s.parse().ok()?;
		return if tmp < 0 {
			let tmp = -tmp as usize;
			Some(total_len - tmp)
		} else {
			Some(tmp as usize)
		};
	}
	if s.starts_with('0') && s.len() != 1 {
		return None;
	}
	if !s.bytes().all(|c| c.is_ascii_digit()) {
		return None;
	}
	s.parse().ok()
}

fn unescape_pointer3<'a>(pointer: &str, buf: &'a mut SmallVec<[u8; 128]>) -> &'a str {
	let mut temp = pointer.as_bytes();
	buf.clear();

	loop {
		if let Some(pos) = memx::memmem(temp, b"~1") {
			buf.extend_from_slice(&temp[..pos]);
			buf.push(b'/');
			temp = &temp[pos + 2..];
			if temp.is_empty() {
				break;
			}
		} else {
			buf.extend_from_slice(temp);
			break;
		}
	}

	let mut cursor = 0;
	while let Some(pos) = memx::memmem(&buf.as_slice()[cursor..], b"~0") {
		cursor += pos;
		buf.remove(cursor + 1);
	}

	unsafe { std::str::from_utf8_unchecked(buf) }
}

/*
// (it's probably faster than most solutions though...)
fn unescape_pointer(pointer: &str, buf: &mut [u8]) -> &'static str {
	// god, what a disgusting mess just to have a non-allocating
	//   "pointer".replace("~1", "/").replace("~0", "~")
	// https://www.rfc-editor.org/rfc/rfc6901.html
	unsafe {
		let mut count = 0;

		let mut expecting = false;
		for byte in pointer.as_bytes().iter() {
			if expecting {
				expecting = false;
				if *byte == b'1' {
					buf[count - 1] = b'/';
					continue;
				}
			}
			expecting = *byte == b'~';
			buf[count] = *byte;
			count += 1;
		}

		let mut expecting = false;
		let mut i = 0;
		while i < count {
			let c = buf[i];
			if expecting {
				expecting = false;
				if c == b'0' {
					let ptr = buf[i..].as_mut_ptr();
					i += 1;
					core::ptr::copy(ptr.add(1), ptr, count - i);
					count -= 1;
					continue;
				}
			}
			expecting = c == b'~';
			i += 1;
		}

		// TODO: LOL.... FUCK FnMut closures and lifetimes...
		std::mem::transmute::<&str, &'static str>(std::str::from_utf8_unchecked(&buf[..count]))
	}
}
*/

///////////////////////////////////////////////////////////////////////////////////////

#[unsafe(no_mangle)]
pub extern "C" fn rust_SRCWRJSON_UnsafePointer(object: &mut SRCWRJSON) -> *mut Value {
	&mut object.v as *mut Value
}

#[unsafe(no_mangle)]
pub extern "C" fn rust_SRCWRJSON_SRCWRJSON(array: bool) -> NonNull<SRCWRJSON> {
	boxval(if array {
		json! {[]}
	} else {
		json! {{}}
	})
}

#[unsafe(no_mangle)]
pub extern "C" fn rust_SRCWRJSON_Clone(object: &mut SRCWRJSON) -> NonNull<SRCWRJSON> {
	boxval(object.v.clone())
}

///////////////////////////////////////////////////////////////////////////////////////

#[unsafe(no_mangle)]
pub extern "C" fn rust_SRCWRJSON_GetObjects(objects: *mut *mut SRCWRJSON, count: isize) {
	for i in 0..count {
		unsafe {
			*objects.offset(i) = boxval(json!({})).as_ptr();
		}
	}
}

///////////////////////////////////////////////////////////////////////////////////////

pub fn rust_SRCWRJSON_ToFile_Inner<T: Write>(
	object: &mut SRCWRJSON,
	mut writer: &mut T,
	flags: i32,
	key: *const c_char,
	keylen: i32,
) -> Option<()> {
	let object = sidx(&mut object.v, key, flags, false, keylen)?;
	if 0 != (flags & J_PRETTY) {
		serde_json::to_writer_pretty(&mut writer, object).ok()?
	} else {
		serde_json::to_writer(&mut writer, object).ok()?
	}
	let _ = write!(
		&mut writer,
		"{}",
		if 0 != (flags & J_FILE_NULL_TERMINATE) {
			"\0"
		} else {
			"\n"
		}
	);
	Some(())
}

#[unsafe(no_mangle)]
pub extern "C" fn rust_SRCWRJSON_ToFile(
	object: &mut SRCWRJSON,
	filename: *const c_char,
	flags: i32,
	key: *const c_char,
	keylen: i32,
	// key: Option<NonNull<c_char>>,
) -> Option<NonZeroU32> {
	let filename = unsafe { CStr::from_ptr(filename) }.to_str().ok()?;
	let mut file = std::fs::OpenOptions::new()
		.write(true)
		.create(0 != (flags & J_FILE_CREATE))
		.append(0 != (flags & J_FILE_APPEND))
		.truncate(0 != (flags & J_FILE_TRUNCATE))
		.open(filename)
		.ok()?;
	rust_SRCWRJSON_ToFile_Inner(object, &mut file, flags, key, keylen)?;
	NonZeroU32::new(1)
}

#[unsafe(no_mangle)]
pub extern "C" fn rust_SRCWRJSON_ToFileHandle(
	object: &mut SRCWRJSON,
	fileobject: &mut IFileObject,
	flags: i32,
	key: *const c_char,
	keylen: i32,
) -> Option<NonZeroU32> {
	rust_SRCWRJSON_ToFile_Inner(object, fileobject, flags, key, keylen)?;
	NonZeroU32::new(1)
}

///////////////////////////////////////////////////////////////////////////////////////

#[unsafe(no_mangle)]
pub extern "C" fn rust_SRCWRJSON_ToString(
	ctx: NonNull<c_void>,
	object: &mut SRCWRJSON,
	buffer: *mut u8,
	local_addr: i32,
	maxlength: usize,
	flags: i32,
	key: *const c_char,
	keylen: i32,
) -> Option<NonZeroUsize> {
	// minimum NUL-terminated buf size for printing Value{1}. Also FUCK YOU!
	if maxlength < 2 {
		return None;
	}

	let object = sidx(&mut object.v, key, flags, false, keylen)?;

	if 0 == (flags & J_TOSTRING_NOALLOC) {
		return match if 0 == (flags & J_PRETTY) {
			serde_json::to_string(object)
		} else {
			serde_json::to_string_pretty(object)
		} {
			Err(_) => None,
			Ok(mut s) => {
				s.push('\0');
				unsafe {
					NonZeroUsize::new(cpp_string_to_local_utf8(
						ctx,
						local_addr,
						maxlength,
						s.as_str().as_ptr(),
					))
				}
			},
		};
	}

	// Okay, well now we write some ugly code to turn the buffer into a writer....

	let buffer = unsafe { std::slice::from_raw_parts_mut(buffer, maxlength) };
	let rangemax = if 0 != (flags & J_DONT_NULL_TERMINATE) {
		maxlength
	} else {
		maxlength - 1
	};
	let mut writer = &mut buffer[..rangemax];

	match if 0 != (flags & J_PRETTY) {
		serde_json::to_writer_pretty(&mut writer, object)
	} else {
		serde_json::to_writer(&mut writer, object)
	} {
		Err(_) => None,
		Ok(_) => {
			let count = maxlength - writer.len();
			if 0 == (flags & J_DONT_NULL_TERMINATE) {
				buffer[count - 1] = 0;
			}
			NonZeroUsize::new(count)
		},
	}
}

///////////////////////////////////////////////////////////////////////////////////////

#[unsafe(no_mangle)]
pub extern "C" fn rust_SRCWRJSON_FromFile(
	filename: *const c_char,
	flags: i32,
) -> Option<NonNull<SRCWRJSON>> {
	let filename = unsafe { CStr::from_ptr(filename) }.to_str().ok()?;

	if 0 != (flags & J_JSON_LINES) {
		// TODO: implement json lines parsing... should be shrimple...
	}

	if 0 == (flags & J_FILE_STOP_AT_ZERO) {
		let content = std::fs::read_to_string(filename).ok()?;
		return Some(boxval(serde_json::from_str(&content).ok()?));
	}

	let file = File::open(filename).ok()?;
	let mut reader = BufReader::new(file);
	let mut buf = Vec::new();

	match reader.read_until(0, &mut buf) {
		Err(_) => None,
		Ok(len) => {
			let rangemax = if len > 0 && buf[buf.len() - 1] == 0 {
				len - 1
			} else {
				len
			};
			Some(boxval(serde_json::from_slice(&buf[..rangemax]).ok()?))
		},
	}
}

// TODO: Duplication... look into `dyn Write` stuff...
#[unsafe(no_mangle)]
pub extern "C" fn rust_SRCWRJSON_FromFileHandle(
	file: &mut IFileObject,
	flags: i32,
) -> Option<NonNull<SRCWRJSON>> {
	if 0 == (flags & J_FILE_STOP_AT_ZERO) {
		let mut content = String::new();
		let _ = file.read_to_string(&mut content).ok()?;
		return Some(boxval(serde_json::from_str(&content).ok()?));
	}

	//let initial_pos = unsafe { IFileObject_Tell(file) };
	//let intial_pos = file.stream_position().ok()?;

	let mut reader = BufReader::with_capacity(4096, file);
	let mut buf = Vec::new();

	let len = reader.read_until(0, &mut buf).ok()?;
	let rangemax = if len > 0 && buf[buf.len() - 1] == 0 {
		len - 1
	} else {
		len
	};
	Some(boxval(serde_json::from_slice(&buf[..rangemax]).ok()?))
}

///////////////////////////////////////////////////////////////////////////////////////

#[unsafe(no_mangle)]
pub extern "C" fn rust_SRCWRJSON_FromString(
	flags: i32,
	s: *const c_char,
	slen: i32,
) -> Option<NonNull<SRCWRJSON>> {
	Some(boxval(
		serde_json::from_str(strxx(s, 0 != (flags & J_UNCHECKED_UTF8_VAL), slen)?).ok()?,
	))
}

///////////////////////////////////////////////////////////////////////////////////////

#[unsafe(no_mangle)]
pub extern "C" fn rust_SRCWRJSON_Has(
	object: &mut SRCWRJSON,
	flags: i32,
	key: *const c_char,
	keylen: i32,
) -> bool {
	// match &object.v {
	//   Value::Object(m) => sidx(&mut object.v, key, flags, false).is_some(),
	//   _ => false,
	// }
	sidx(&mut object.v, key, flags, false, keylen).is_some()
}

#[unsafe(no_mangle)]
pub extern "C" fn rust_SRCWRJSON_GetType(
	object: &mut SRCWRJSON,
	flags: i32,
	key: *const c_char,
	keylen: i32,
) -> J_Type {
	let Some(object) = sidx(&mut object.v, key, flags, false, keylen) else {
		return J_Type::J_Unknown;
	};
	match object {
		Value::Null => J_Type::J_Null,
		Value::Bool(_) => J_Type::J_Bool,
		Value::Number(_) => J_Type::J_Number,
		Value::String(_) => J_Type::J_String,
		Value::Array(_) => J_Type::J_Array,
		Value::Object(_) => J_Type::J_Map,
	}
}

#[unsafe(no_mangle)]
pub extern "C" fn rust_SRCWRJSON_IsArray(
	object: &mut SRCWRJSON,
	flags: i32,
	key: *const c_char,
	keylen: i32,
) -> bool {
	let Some(object) = sidx(&mut object.v, key, flags, false, keylen) else {
		return false;
	};
	object.is_array()
}

///////////////////////////////////////////////////////////////////////////////////////

#[unsafe(no_mangle)]
pub extern "C" fn rust_SRCWRJSON_len(
	object: &mut SRCWRJSON,
	flags: i32,
	key: *const c_char,
	keylen: i32,
) -> usize {
	let Some(object) = sidx(&mut object.v, key, flags, false, keylen) else {
		return -1i32 as usize;
	};
	match object {
		Value::Object(m) => m.len(),
		Value::Array(a) => a.len(),
		Value::String(s) => s.len(),
		_ => 0,
	}
}

///////////////////////////////////////////////////////////////////////////////////////

#[unsafe(no_mangle)]
pub extern "C" fn rust_SRCWRJSON_Get(
	object: &mut SRCWRJSON,
	flags: i32,
	key: *const c_char,
	keylen: i32,
) -> Option<NonNull<SRCWRJSON>> {
	let x = sidx(&mut object.v, key, flags, false, keylen)?;
	Some(boxval(if 0 != (flags & J_MOVE) {
		std::mem::take(x)
	} else {
		x.clone()
	}))
}

#[unsafe(no_mangle)]
pub extern "C" fn rust_SRCWRJSON_GetIdx(
	object: &mut SRCWRJSON,
	flags: i32,
	idx: usize,
) -> Option<NonNull<SRCWRJSON>> {
	let x = GetIdx_Inner(&mut object.v, idx, flags)?;
	Some(boxval(if 0 != (flags & J_MOVE) {
		std::mem::take(x)
	} else {
		x.clone()
	}))
}

///////////////////////////////////////////////////////////////////////////////////////

#[unsafe(no_mangle)]
pub extern "C" fn rust_SRCWRJSON_Set(
	object: &mut SRCWRJSON,
	other: &mut SRCWRJSON,
	flags: i32,
	key: *const c_char,
	keylen: i32,
) -> Option<NonZeroU32> {
	let v = sidx(&mut object.v, key, flags, true, keylen)?;
	if 0 != (flags & J_MOVE) {
		std::mem::swap(v, &mut other.v);
		other.v = Value::Null;
	} else {
		*v = other.v.clone();
	}
	NonZeroU32::new(1)
}

fn GetIdx_Inner(v: &mut Value, mut idx: usize, _flags: i32) -> Option<&mut Value> {
	let v = v.as_array_mut()?;
	if (idx as i32) < 0i32 {
		let x = -(idx as i32) as usize;
		if x != 1 && x > v.len() {
			return None;
		}
		idx = v.len() - x;
	}
	Some(v.get_mut(idx).unwrap())
}

fn SetIdx_Inner(
	v: &mut Value,
	mut idx: usize,
	other: &mut Value,
	flags: i32,
) -> Option<NonZeroU32> {
	let v = v.as_array_mut()?;
	if (idx as i32) < 0i32 {
		let x = -(idx as i32) as usize;
		if x != 1 && x > v.len() {
			return None;
		}
		idx = v.len() - x;
	}
	if v.len() < idx + 1 {
		v.resize(idx + 1, Value::Null);
	}
	let ret = if 0 != (flags & J_IS_DIFFERENT) && !v[idx].eq(other) {
		None
	} else {
		NonZeroU32::new(1)
	};
	if 0 != (flags & J_MOVE) {
		v[idx] = std::mem::take(other);
	} else {
		v[idx] = other.clone();
	}
	ret
}

#[unsafe(no_mangle)]
pub extern "C" fn rust_SRCWRJSON_SetIdx(
	object: &mut SRCWRJSON,
	idx: usize,
	other: &mut SRCWRJSON,
	flags: i32,
) -> Option<NonZeroU32> {
	SetIdx_Inner(&mut object.v, idx, &mut other.v, flags)
}

///////////////////////////////////////////////////////////////////////////////////////

#[unsafe(no_mangle)]
pub extern "C" fn rust_SRCWRJSON_SetFromString(
	object: &mut SRCWRJSON,
	s: *const c_char,
	end: i32,
	flags: i32,
	key: *const c_char,
	keylen: i32,
) -> Option<NonZeroU32> {
	let mut other =
		serde_json::from_str(strxx(s, 0 != (flags & J_UNCHECKED_UTF8_VAL), end)?).ok()?;
	Setx_Inner(&mut object.v, &mut other, flags, key, keylen)
}

#[unsafe(no_mangle)]
pub extern "C" fn rust_SRCWRJSON_SetFromStringIdx(
	object: &mut SRCWRJSON,
	s: *const c_char,
	end: i32,
	flags: i32,
	idx: usize,
) -> Option<NonZeroU32> {
	let other = strxx(s, 0 != (flags & J_UNCHECKED_UTF8_VAL), end)?;
	SetIdx_Inner(
		&mut object.v,
		idx,
		&mut serde_json::from_str(other).ok()?,
		flags | J_MOVE,
	)
}

///////////////////////////////////////////////////////////////////////////////////////

// relatively stripped/trimmed out of sidx()....
fn sidxremove(object: &mut SRCWRJSON, s: *const c_char, flags: i32, keylen: i32) -> Option<()> {
	let key = strxx(s, 0 != (flags & J_UNCHECKED_UTF8_KEY), keylen)?;
	if 0 != (flags & J_NO_POINTER) || key.as_bytes()[0] != b'/' {
		let m = object.v.as_object_mut()?;
		let _ = m.remove(key)?;
		return Some(());
	}

	let mut parents: SmallVec<[&str; 8]> =
		key.split('/').skip(1).filter(|&x| !x.is_empty()).collect();
	let base = parents.pop()?;

	let mut scratch = SmallVec::<[u8; 128]>::new();
	let mut current_value = &mut object.v;

	/*
	let rrr = parents.iter().try_fold(
		(scratch, current_value),
		|(mut scratch, current_value), p| {
			let k = unescape_pointer3(p, &mut scratch);
			match current_value {
				Value::Array(a) => as_array_idx(k, a.len())
					.and_then(|i| a.get_mut(i).and_then(|v| Some((scratch, v)))),
				Value::Object(m) => m.get_mut(k).and_then(|v| Some((scratch, v))),
				_ => None,
			}
		},
	);
	*/

	for p in &parents {
		let k = unescape_pointer3(p, &mut scratch);
		current_value = match current_value {
			Value::Array(a) => as_array_idx(k, a.len()).and_then(|i| a.get_mut(i)),
			Value::Object(m) => m.get_mut(k),
			_ => None,
		}?;
	}
	if let Value::Array(x) = current_value {
		let xlen = x.len();
		let idx = as_array_idx(base, xlen)?;
		if idx >= xlen {
			return None;
		}
		if 0 != (flags & J_SWAP_REMOVE) {
			x.swap_remove(idx);
		} else {
			x.remove(idx);
		}
	} else {
		let _ = current_value.as_object_mut()?.remove(base)?;
	}
	Some(())
}

#[unsafe(no_mangle)]
pub extern "C" fn rust_SRCWRJSON_Remove(
	object: &mut SRCWRJSON,
	flags: i32,
	key: *const c_char,
	keylen: i32,
) -> Option<NonZeroU32> {
	sidxremove(object, key, flags, keylen)?;
	NonZeroU32::new(1)
}

#[unsafe(no_mangle)]
pub extern "C" fn rust_SRCWRJSON_RemoveIdx(
	object: &mut SRCWRJSON,
	flags: i32,
	idx: usize,
) -> Option<NonZeroU32> {
	let v = object.v.as_array_mut()?;
	let idx = if (idx as i32) < 0i32 {
		let x = -(idx as i32) as usize;
		if x != 1 && x > v.len() {
			return None;
		}
		v.len() - x
	} else {
		idx
	};
	if idx >= v.len() {
		return None;
	}
	if 0 != (flags & J_SWAP_REMOVE) {
		v.swap_remove(idx);
	} else {
		v.remove(idx);
	}
	NonZeroU32::new(1)
}

///////////////////////////////////////////////////////////////////////////////////////

// TODO: RemoveAllWithSelector... Implement? Remove? The existing Selector crates are kind of ass...
#[allow(unused_variables)]
#[unsafe(no_mangle)]
pub extern "C" fn rust_SRCWRJSON_RemoveAllWithSelector(
	object: &mut SRCWRJSON,
	selectorpath: *const c_char,
	flags: i32,
	key: *const c_char,
	keylen: i32,
) -> Option<NonZeroU32> {
	/*
	let x = sidx(&mut object.v, key, flags, false)?;
	let selectorpath = strxx(selectorpath, 0 != (flags & J_UNCHECKED_UTF8_KEY), 0)?;
	let selector = SelectorMut::default().str_path(selectorpath).ok()?;

	let mut count: i32 = 0;

	selector.replace_with(&mut |_| Some(Value::Null));

	/*
	while let Ok(v) = jsonpath_lib::delete(x, selectorpath) {
		count += 1;
	}
	*/
	*/
	None
}

///////////////////////////////////////////////////////////////////////////////////////

#[unsafe(no_mangle)]
pub extern "C" fn rust_SRCWRJSON_Clear(
	object: &mut SRCWRJSON,
	flags: i32,
	key: *const c_char,
	keylen: i32,
) -> Option<NonZeroU32> {
	let object = sidx(&mut object.v, key, flags, false, keylen)?;
	match object {
		Value::Array(a) => a.clear(),
		Value::Object(m) => m.clear(),
		Value::String(s) => s.clear(),
		Value::Number(n) => *n = 0.into(),
		Value::Bool(b) => *b = false,
		Value::Null => (),
	}
	NonZeroU32::new(1)
}

///////////////////////////////////////////////////////////////////////////////////////

#[unsafe(no_mangle)]
pub extern "C" fn rust_SRCWRJSON_ReplaceCell(
	object: &mut SRCWRJSON,
	newcell: i32,
	flags: i32,
	key: *const c_char,
	keylen: i32,
) -> Option<NonZeroI32> {
	let object = sidx(&mut object.v, key, flags, false, keylen)?;
	let x = object.as_i64()? as i32;
	*object = newcell.into();
	NonZeroI32::new(x)
}

#[unsafe(no_mangle)]
pub extern "C" fn rust_SRCWRJSON_ReplaceCellIdx(
	object: &mut SRCWRJSON,
	newcell: i32,
	flags: i32,
	idx: usize,
) -> Option<NonZeroI32> {
	let array = GetIdx_Inner(&mut object.v, idx, flags)?;
	let x = array.get_mut(idx)?;
	let ret = x.as_i64()? as i32;
	*x = newcell.into();
	//let x = object.as_i64()? as i32;
	//*object = newcell.into();
	NonZeroI32::new(ret)
}

///////////////////////////////////////////////////////////////////////////////////////

#[unsafe(no_mangle)]
pub extern "C" fn rust_SRCWRJSON_SetZss(
	object: &mut SRCWRJSON,
	flags: i32,
	key: *const c_char,
	other: &mut SRCWRJSON,
	key2: *const c_char,
	key2len: i32,
) -> Option<NonZeroI32> {
	let other = sidx(&mut other.v, key2, flags, false, key2len)?;
	let object = sidx(&mut object.v, key, flags, true, 0)?;
	*object = other.clone();
	const { Some(unsafe { NonZeroI32::new_unchecked(1) }) }
}

///////////////////////////////////////////////////////////////////////////////////////

fn Struct_inner(
	object: &mut Value,
	mut buf: *mut u32,
	format: *const c_char,
	flags: i32,
	setter: bool,
) -> Option<NonZeroI32> {
	let format = strxx(format, 0 != (flags & J_UNCHECKED_UTF8_VAL), 0)?;

	for kvpair in format.split('|') {
		let mut thing = kvpair.split(',');
		let jsonkey = thing.next()?;
		let sptype = thing.next()?;
		let n = thing.next();
		let v = sidx(
			object,
			jsonkey.as_ptr() as *const i8,
			0,
			setter,
			jsonkey.len() as i32,
		)?;
		if setter {
			if sptype == "char" {
				let n: usize = n?.parse().ok()?;
				let s = strxx(buf as *const i8, false, 0)?;
				*v = s.into();
				unsafe {
					buf = buf.add((n + 3) / 4);
				}
			} else {
				let mut mem_to_json = |v: &mut Value| {
					match sptype {
						"int" => unsafe {
							*v = (*buf as i32).into();
						},
						"float" => unsafe {
							*v = f32::from_bits(*buf).into();
						},
						"bool" => unsafe {
							*v = (*buf != 0).into();
						},
						_ => (),
					}
					unsafe {
						buf = buf.add(1);
					}
				};
				if let Some(n) = n {
					let n = n.parse::<usize>().ok()?;
					*v = Value::Array(vec![Value::Null; n]);
					for i in 0..n {
						mem_to_json(&mut v[i]);
					}
				} else {
					mem_to_json(v);
				}
			};
			continue;
		}
		// special case because strings can be annoying...
		if sptype == "char" {
			let n = n.unwrap_or("1").parse().ok()?;
			let s = v.as_str()?;
			write_to_sp_buf(s.as_bytes(), None, buf as *mut u8, 0, n, 0)?;
			unsafe {
				buf = buf.add((n + 3) / 4);
			}
			continue;
		}
		let mut json_to_mem = |v: &Value| {
			unsafe {
				*buf = match sptype {
					"bool" => {
						if v.as_bool()? {
							1
						} else {
							0
						}
					},
					"int" => v.as_i64()? as i32 as u32,
					"float" => (v.as_f64()? as f32).to_bits(),
					_ => return None,
				};
				buf = buf.add(1);
			}
			Some(())
		};
		if let Some(n) = n {
			// array....
			let n = n.parse().ok()?;
			let v = v.as_array_mut()?;
			for i in 0..n {
				json_to_mem(v.get(i)?)?;
			}
		} else {
			json_to_mem(v)?;
		}
	}
	None
}

#[unsafe(no_mangle)]
pub extern "C" fn rust_SRCWRJSON_GetStruct(
	object: &mut SRCWRJSON,
	buf: *mut u32,
	format: *const c_char,
	flags: i32,
	key: *const c_char,
	keylen: i32,
) -> Option<NonZeroI32> {
	let object = sidx(&mut object.v, key, flags, false, keylen)?;
	Struct_inner(object, buf, format, flags, false)
}

#[unsafe(no_mangle)]
pub extern "C" fn rust_SRCWRJSON_GetStructIdx(
	object: &mut SRCWRJSON,
	buf: *mut u32,
	format: *const c_char,
	flags: i32,
	idx: usize,
) -> Option<NonZeroI32> {
	let object = GetIdx_Inner(&mut object.v, idx, flags)?;
	Struct_inner(object, buf, format, flags, false)
}

#[unsafe(no_mangle)]
pub extern "C" fn rust_SRCWRJSON_SetStruct(
	object: &mut SRCWRJSON,
	buf: *mut u32,
	format: *const c_char,
	flags: i32,
	key: *const c_char,
	keylen: i32,
) -> Option<NonZeroI32> {
	let object = sidx(&mut object.v, key, flags, true, keylen)?;
	Struct_inner(object, buf, format, flags, true)
}
#[unsafe(no_mangle)]
pub extern "C" fn rust_SRCWRJSON_SetStructIdx(
	object: &mut SRCWRJSON,
	buf: *mut u32,
	format: *const c_char,
	flags: i32,
	idx: usize,
) -> Option<NonZeroU32> {
	// TODO: Make better? I don't really like how this right now...
	let mut structy = json!({});
	Struct_inner(&mut structy, buf, format, flags, true)?;
	SetIdx_Inner(&mut object.v, idx, &mut structy, flags | J_MOVE)
}

///////////////////////////////////////////////////////////////////////////////////////

#[unsafe(no_mangle)]
pub extern "C" fn rust_SRCWRJSON_GetCell(
	object: &mut SRCWRJSON,
	flags: i32,
	key: *const c_char,
	keylen: i32,
) -> Option<NonZeroI32> {
	NonZeroI32::new(sidx(&mut object.v, key, flags, false, keylen)?.as_i64()? as i32)
}

#[unsafe(no_mangle)]
pub extern "C" fn rust_SRCWRJSON_GetCellIdx(
	object: &mut SRCWRJSON,
	flags: i32,
	idx: usize,
) -> Option<NonZeroI32> {
	NonZeroI32::new(GetIdx_Inner(&mut object.v, idx, flags)?.as_i64()? as i32)
}

#[unsafe(no_mangle)]
pub extern "C" fn rust_SRCWRJSON_SetCell(
	object: &mut SRCWRJSON,
	value: i32,
	flags: i32,
	key: *const c_char,
	keylen: i32,
) -> Option<NonZeroU32> {
	Setx_Inner(&mut object.v, &mut value.into(), flags, key, keylen)
}

#[unsafe(no_mangle)]
pub extern "C" fn rust_SRCWRJSON_SetCellIdx(
	object: &mut SRCWRJSON,
	value: i32,
	flags: i32,
	idx: usize,
) -> Option<NonZeroU32> {
	SetIdx_Inner(&mut object.v, idx, &mut value.into(), flags)
}

///////////////////////////////////////////////////////////////////////////////////////

#[unsafe(no_mangle)]
pub extern "C" fn rust_SRCWRJSON_GetF32(
	object: &mut SRCWRJSON,
	flags: i32,
	key: *const c_char,
	keylen: i32,
) -> Option<NonZeroU32> {
	NonZeroU32::new((sidx(&mut object.v, key, flags, false, keylen)?.as_f64()? as f32).to_bits())
}

#[unsafe(no_mangle)]
pub extern "C" fn rust_SRCWRJSON_GetF32Idx(
	object: &mut SRCWRJSON,
	flags: i32,
	idx: usize,
) -> Option<NonZeroU32> {
	NonZeroU32::new((GetIdx_Inner(&mut object.v, idx, flags)?.as_f64()? as f32).to_bits())
}

#[unsafe(no_mangle)]
pub extern "C" fn rust_SRCWRJSON_SetF32(
	object: &mut SRCWRJSON,
	value: f32,
	flags: i32,
	key: *const c_char,
	keylen: i32,
) -> Option<NonZeroU32> {
	Setx_Inner(&mut object.v, &mut value.into(), flags, key, keylen)
}

#[unsafe(no_mangle)]
pub extern "C" fn rust_SRCWRJSON_SetF32Idx(
	object: &mut SRCWRJSON,
	value: f32,
	flags: i32,
	idx: usize,
) -> Option<NonZeroU32> {
	SetIdx_Inner(&mut object.v, idx, &mut value.into(), flags)
}

///////////////////////////////////////////////////////////////////////////////////////

#[unsafe(no_mangle)]
pub extern "C" fn rust_SRCWRJSON_GetBool(
	object: &mut SRCWRJSON,
	flags: i32,
	key: *const c_char,
	keylen: i32,
) -> Option<NonZeroU32> {
	sidx(&mut object.v, key, flags, false, keylen)?
		.as_bool()?
		.then_some(const { unsafe { NonZeroU32::new_unchecked(1) } })
}

#[unsafe(no_mangle)]
pub extern "C" fn rust_SRCWRJSON_GetBoolIdx(
	object: &mut SRCWRJSON,
	flags: i32,
	idx: usize,
) -> Option<NonZeroU32> {
	GetIdx_Inner(&mut object.v, idx, flags)?
		.as_bool()?
		.then_some(const { unsafe { NonZeroU32::new_unchecked(1) } })
}

#[unsafe(no_mangle)]
pub extern "C" fn rust_SRCWRJSON_SetBool(
	object: &mut SRCWRJSON,
	value: bool,
	flags: i32,
	key: *const c_char,
	keylen: i32,
) -> Option<NonZeroU32> {
	Setx_Inner(&mut object.v, &mut value.into(), flags, key, keylen)
}

#[unsafe(no_mangle)]
pub extern "C" fn rust_SRCWRJSON_SetBoolIdx(
	object: &mut SRCWRJSON,
	value: bool,
	flags: i32,
	idx: usize,
) -> Option<NonZeroU32> {
	SetIdx_Inner(&mut object.v, idx, &mut value.into(), flags)
}

fn rust_SRCWRJSON_ToggleBool_Inner(v: &mut Value) -> Option<NonZeroU32> {
	match v {
		Value::Null => *v = Value::Bool(true),
		Value::Bool(x) => {
			*x = !*x;
			if !*x {
				return None;
			}
		},
		_ => return None,
	}
	NonZeroU32::new(1)
}

#[unsafe(no_mangle)]
pub extern "C" fn rust_SRCWRJSON_ToggleBool(
	object: &mut SRCWRJSON,
	flags: i32,
	key: *const c_char,
	keylen: i32,
) -> Option<NonZeroU32> {
	let v = sidx(&mut object.v, key, flags, true, keylen)?;
	rust_SRCWRJSON_ToggleBool_Inner(v)
}

#[unsafe(no_mangle)]
pub extern "C" fn rust_SRCWRJSON_ToggleBoolIdx(
	object: &mut SRCWRJSON,
	flags: i32,
	idx: usize,
) -> Option<NonZeroU32> {
	rust_SRCWRJSON_ToggleBool_Inner(GetIdx_Inner(&mut object.v, idx, flags)?)
}

///////////////////////////////////////////////////////////////////////////////////////

#[unsafe(no_mangle)]
pub extern "C" fn rust_SRCWRJSON_GetI64(
	object: &mut SRCWRJSON,
	buffer: &mut i64,
	flags: i32,
	key: *const c_char,
	keylen: i32,
) -> Option<NonZeroI32> {
	let v = sidx(&mut object.v, key, flags, false, keylen)?.as_i64()?;
	*buffer = v;
	NonZeroI32::new(v as i32)
}

#[unsafe(no_mangle)]
pub extern "C" fn rust_SRCWRJSON_GetI64Idx(
	object: &mut SRCWRJSON,
	buffer: &mut i64,
	flags: i32,
	idx: usize,
) -> Option<NonZeroI32> {
	let v = GetIdx_Inner(&mut object.v, idx, flags)?.as_i64()?;
	*buffer = v;
	NonZeroI32::new(v as i32)
}

#[unsafe(no_mangle)]
pub extern "C" fn rust_SRCWRJSON_SetI64(
	object: &mut SRCWRJSON,
	value: i64,
	flags: i32,
	key: *const c_char,
	keylen: i32,
) -> Option<NonZeroU32> {
	Setx_Inner(&mut object.v, &mut value.into(), flags, key, keylen)
}

#[unsafe(no_mangle)]
pub extern "C" fn rust_SRCWRJSON_SetI64Idx(
	object: &mut SRCWRJSON,
	value: i64,
	flags: i32,
	idx: usize,
) -> Option<NonZeroU32> {
	SetIdx_Inner(&mut object.v, idx, &mut value.into(), flags)
}

///////////////////////////////////////////////////////////////////////////////////////

#[unsafe(no_mangle)]
pub extern "C" fn rust_SRCWRJSON_GetF64(
	object: &mut SRCWRJSON,
	buffer: &mut f64,
	flags: i32,
	key: *const c_char,
	keylen: i32,
) -> Option<NonZeroU32> {
	let v = sidx(&mut object.v, key, flags, false, keylen)?.as_f64()?;
	*buffer = v;
	NonZeroU32::new((v as f32).to_bits())
}

#[unsafe(no_mangle)]
pub extern "C" fn rust_SRCWRJSON_GetF64Idx(
	object: &mut SRCWRJSON,
	buffer: &mut f64,
	flags: i32,
	idx: usize,
) -> Option<NonZeroU32> {
	let v = GetIdx_Inner(&mut object.v, idx, flags)?.as_f64()?;
	*buffer = v;
	NonZeroU32::new((v as f32).to_bits())
}

#[unsafe(no_mangle)]
pub extern "C" fn rust_SRCWRJSON_SetF64(
	object: &mut SRCWRJSON,
	value: f64,
	flags: i32,
	key: *const c_char,
	keylen: i32,
) -> Option<NonZeroU32> {
	Setx_Inner(&mut object.v, &mut value.into(), flags, key, keylen)
}

#[unsafe(no_mangle)]
pub extern "C" fn rust_SRCWRJSON_SetF64Idx(
	object: &mut SRCWRJSON,
	value: f64,
	flags: i32,
	idx: usize,
) -> Option<NonZeroU32> {
	SetIdx_Inner(&mut object.v, idx, &mut value.into(), flags)
}

///////////////////////////////////////////////////////////////////////////////////////

#[unsafe(no_mangle)]
pub extern "C" fn rust_SRCWRJSON_IsNull(
	object: &mut SRCWRJSON,
	flags: i32,
	key: *const c_char,
	keylen: i32,
) -> Option<NonZeroU32> {
	sidx(&mut object.v, key, flags, false, keylen)?
		.as_null()
		.map(|_| const { unsafe { NonZeroU32::new_unchecked(1) } })
}

#[unsafe(no_mangle)]
pub extern "C" fn rust_SRCWRJSON_IsNullIdx(
	object: &mut SRCWRJSON,
	_flags: i32,
	idx: usize,
) -> Option<NonZeroU32> {
	object
		.v
		.as_array()?
		.get(idx)?
		.as_null()
		.map(|_| const { unsafe { NonZeroU32::new_unchecked(1) } })
}

#[unsafe(no_mangle)]
pub extern "C" fn rust_SRCWRJSON_SetNull(
	object: &mut SRCWRJSON,
	flags: i32,
	key: *const c_char,
	keylen: i32,
) -> Option<NonZeroU32> {
	Setx_Inner(&mut object.v, &mut Value::Null, flags, key, keylen)
}

#[unsafe(no_mangle)]
pub extern "C" fn rust_SRCWRJSON_SetNullIdx(
	object: &mut SRCWRJSON,
	flags: i32,
	idx: usize,
) -> Option<NonZeroU32> {
	SetIdx_Inner(&mut object.v, idx, &mut Value::Null, flags)
}

///////////////////////////////////////////////////////////////////////////////////////

#[unsafe(no_mangle)]
pub extern "C" fn rust_SRCWRJSON_GetString(
	ctx: NonNull<c_void>,
	object: &mut SRCWRJSON,
	buffer: *mut u8,
	local_addr: i32,
	maxlength: usize,
	flags: i32,
	key: *const c_char,
	keylen: i32,
) -> Option<NonZeroUsize> {
	// FUCK YOU!
	if maxlength < 2 {
		return None;
	}

	let s = sidx(&mut object.v, key, flags, false, keylen)?.as_str()?;
	write_to_sp_buf(
		s.as_bytes(),
		Some(ctx),
		buffer,
		local_addr,
		maxlength,
		flags,
	)
}

#[unsafe(no_mangle)]
pub extern "C" fn rust_SRCWRJSON_GetStringIdx(
	ctx: NonNull<c_void>,
	object: &mut SRCWRJSON,
	buffer: *mut u8,
	local_addr: i32,
	maxlength: usize,
	flags: i32,
	idx: usize,
) -> Option<NonZeroUsize> {
	// FUCK YOU! buffer is not big enough for a single character + a NUL terminator.
	if (maxlength as i32) < 2 {
		return None;
	}

	let s = GetIdx_Inner(&mut object.v, idx, flags)?.as_str()?;
	write_to_sp_buf(
		s.as_bytes(),
		Some(ctx),
		buffer,
		local_addr,
		maxlength,
		flags,
	)
}

///////////////////////////////////////////////////////////////////////////////////////

#[unsafe(no_mangle)]
pub extern "C" fn rust_SRCWRJSON_SetString(
	object: &mut SRCWRJSON,
	s: *mut c_char,
	end: i32,
	flags: i32,
	key: *const c_char,
	keylen: i32,
) -> Option<NonZeroU32> {
	let s = strxx(s, 0 != (flags & J_UNCHECKED_UTF8_VAL), end)?;
	Setx_Inner(&mut object.v, &mut s.into(), flags, key, keylen)
}

#[unsafe(no_mangle)]
pub extern "C" fn rust_SRCWRJSON_SetStringIdx(
	object: &mut SRCWRJSON,
	s: *mut c_char,
	end: i32,
	flags: i32,
	idx: usize,
) -> Option<NonZeroU32> {
	let s = strxx(s, 0 != (flags & J_UNCHECKED_UTF8_VAL), end)?;
	SetIdx_Inner(&mut object.v, idx, &mut s.into(), flags | J_MOVE)
}

///////////////////////////////////////////////////////////////////////////////////////

#[unsafe(no_mangle)]
pub extern "C" fn rust_SRCWRJSON_FillKeys(
	object: &mut SRCWRJSON,
	cellarray: &mut ICellArray,
	flags: i32,
	key: *const c_char,
	keylen: i32,
) -> Option<NonZeroU32> {
	let object = sidx(&mut object.v, key, flags, false, keylen)?.as_object()?;
	for k in object.keys() {
		cellarray.push_string(k)?;
	}
	NonZeroU32::new(1)
}