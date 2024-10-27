// SPDX-License-Identifier: GPL-3.0-or-later
// Copyright 2022 rtldg <rtldg@protonmail.com>
// This file is part of srcwrtimer (https://github.com/srcwr/srcwrtimer/)

use core::ffi::c_char;
use core::ptr::NonNull;
use std::io::BufReader;
use std::io::Read;
use std::net::IpAddr;
use std::num::NonZeroU32;
use std::str::FromStr;

use extshared::IFileObject::IFileObject;
use extshared::*;

use crate::*;

unsafe extern "C" {
	fn strlen(s: *const c_char) -> usize;
}

///////////////////////////////////////////////////////////////////////////////////////

#[unsafe(no_mangle)]
pub extern "C" fn rust_handle_destroy_SRCWRHTTPReq(object: *mut SRCWRHTTPReq) {
	// println!("destroy SRCWRHTTPReq {:#?}", object);
	let object = unsafe { Box::from_raw(object) };
	/*
	unsafe {
		if let Some(forward) = object._forward {
			extshared::cpp_forward_release(forward);
		}
	}
	*/
	drop(object);
}

#[unsafe(no_mangle)]
pub extern "C" fn rust_handle_size_SRCWRHTTPReq(object: &SRCWRHTTPReq, size: &mut u32) -> bool {
	let mut s = size_of::<SRCWRHTTPReq>();
	if let Some(headers) = &object.headers {
		for (key, value) in headers.iter() {
			s += value.as_bytes().len() + key.as_str().len(); // not strictly accurate but none of this is anyway...
		}
	}
	if let Some(b) = &object.body {
		s += b.capacity();
	}
	if let Some(b) = &object.body_file_on_send {
		s += b.capacity();
	}
	if let Some(b) = &object.basic_auth {
		s += b.0.capacity();
		if let Some(p) = &b.1 {
			s += p.capacity();
		}
	}
	if let Some(d) = &object.cbinfo.download_path {
		s += d.capacity();
	}
	if let Some(u) = &object.url {
		s += u.as_str().len(); // not strictly accurate but none of this is anyway...
	}
	*size = s as u32;
	true
}

#[unsafe(no_mangle)]
pub extern "C" fn rust_handle_destroy_SRCWRHTTPResp(object: *mut SRCWRHTTPResp) {
	// println!("destroy SRCWRHTTPResp {:#?}", object);
	let object = unsafe { Box::from_raw(object) };
	/*
	unsafe {
		if let Some(forward) = object.cbinfo.forward {
			extshared::cpp_forward_release(forward);
		}
	}
	*/
	drop(object);
}

#[unsafe(no_mangle)]
pub extern "C" fn rust_handle_size_SRCWRHTTPResp(object: &SRCWRHTTPResp, size: &mut u32) -> bool {
	let mut s = size_of::<SRCWRHTTPResp>();
	for (key, value) in object.headers.iter() {
		s += value.as_bytes().len() + key.as_str().len(); // not strictly accruate but none of this is anyway...
	}
	if let Some(t) = &object.text {
		s += t.capacity();
	}
	if let Some(e) = &object.e {
		s += e.capacity();
	}
	if let Some(d) = &object.cbinfo.download_path {
		s += d.capacity();
	}
	s += object.bytes.len();
	*size = s as u32;
	true
}

///////////////////////////////////////////////////////////////////////////////////////

#[unsafe(no_mangle)]
pub extern "C" fn rust_SRCWRHTTPReq_SRCWRHTTPReq(
	url: *const c_char,
) -> Option<NonNull<SRCWRHTTPReq>> {
	let url = reqwest::Url::parse(strxx(url, false, 0)?).ok()?;

	let boxed = Box::new(SRCWRHTTPReq {
		url:     Some(url),
		method:  reqwest::Method::GET,
		headers: None, //Default::default(),

		body:              None, //Vec::new(),
		body_file_on_send: None,

		local_address:       None,
		basic_auth:          None,
		allow_invalid_certs: false,

		max_redirections: 10,
		timeout:          30000,
		connect_timeout:  10000,

		cbinfo: CallbackInfo {
			user_value:    0,
			forward:       None,
			download_path: None,
		},
	});
	Some(unsafe { NonNull::new_unchecked(Box::into_raw(boxed)) })
}

///////////////////////////////////////////////////////////////////////////////////////

#[unsafe(no_mangle)]
pub extern "C" fn rust_SRCWRHTTPReq_YEET(
	object: &mut SRCWRHTTPReq,
	forward: Option<NonNull<c_void>>,
	value: i32,
	method: *const c_char,
) {
	if let Some(method) = strxx(method, true, 0) {
		if let Ok(method) = reqwest::Method::from_str(method) {
			object.method = method;
		}
	}
	object.cbinfo.user_value = value;
	object.cbinfo.forward = forward;
	http_thread::send_to_request_builder_thread(ToAsyncThread::Req(std::mem::take(object)));
}

#[unsafe(no_mangle)]
pub extern "C" fn rust_SRCWRHTTPReq_Download(
	object: &mut SRCWRHTTPReq,
	forward: Option<NonNull<c_void>>,
	value: i32,
	filename: *const c_char,
) -> Option<NonZeroU32> {
	object.cbinfo.download_path = Some(strxx(filename, false, 0)?.into());
	rust_SRCWRHTTPReq_YEET(object, forward, value, core::ptr::null());
	NonZeroU32::new(1)
}

///////////////////////////////////////////////////////////////////////////////////////

#[unsafe(no_mangle)]
pub extern "C" fn rust_SRCWRHTTPReq_method(
	object: &mut SRCWRHTTPReq,
	method: *const c_char,
) -> Option<NonZeroU32> {
	object.method = reqwest::Method::from_str(strxx(method, true, 0)?).ok()?;
	NonZeroU32::new(1)
}

#[unsafe(no_mangle)]
pub extern "C" fn rust_SRCWRHTTPReq_header(
	object: &mut SRCWRHTTPReq,
	key: *const c_char,
	value: *const c_char,
) -> Option<NonZeroU32> {
	object
		.headers
		.get_or_insert_default()
		.insert(strxx(key, false, 0)?, strxx(value, false, 0)?.parse().ok()?);
	NonZeroU32::new(1)
}

#[unsafe(no_mangle)]
pub extern "C" fn rust_SRCWRHTTPReq_query_param(
	object: &mut SRCWRHTTPReq,
	key: *const c_char,
	value: *const c_char,
) -> Option<NonZeroU32> {
	let key = strxx(key, false, 0)?;
	let mut pairs = object.url.as_mut().unwrap().query_pairs_mut();
	if let Some(value) = strxx(value, false, 0) {
		pairs.append_pair(key, value);
	} else {
		pairs.append_key_only(key);
	}
	NonZeroU32::new(1)
}

///////////////////////////////////////////////////////////////////////////////////////

#[unsafe(no_mangle)]
pub extern "C" fn rust_SRCWRHTTPReq_body_set(
	object: &mut SRCWRHTTPReq,
	end: i32,
	content: *const i8,
) -> Option<NonZeroU32> {
	object.body.get_or_insert_default().clear();
	rust_SRCWRHTTPReq_body_add(object, end, content)
}

#[unsafe(no_mangle)]
pub extern "C" fn rust_SRCWRHTTPReq_body_add_file_on_send(
	object: &mut SRCWRHTTPReq,
	path: *const c_char,
) -> Option<NonZeroU32> {
	object.body_file_on_send = Some(strxx(path, false, 0)?.to_string());
	NonZeroU32::new(1)
}

#[unsafe(no_mangle)]
pub extern "C" fn rust_SRCWRHTTPReq_body_add(
	object: &mut SRCWRHTTPReq,
	end: i32,
	content: *const i8,
) -> Option<NonZeroU32> {
	let len = if end < 1 {
		unsafe { strlen(content) }
	} else {
		end as usize
	};
	let content = unsafe { std::slice::from_raw_parts(content as *const u8, len) };
	object
		.body
		.get_or_insert_default()
		.extend_from_slice(content);
	NonZeroU32::new(1)
}

#[unsafe(no_mangle)]
pub extern "C" fn rust_SRCWRHTTPReq_body_add_file(
	object: &mut SRCWRHTTPReq,
	path: *const c_char,
) -> Option<NonZeroU32> {
	let path = strxx(path, false, 0)?;
	let mut f = std::fs::File::open(path).ok()?;
	let _count = f.read_to_end(object.body.get_or_insert_default()).ok()?;
	NonZeroU32::new(1)
}

#[unsafe(no_mangle)]
pub extern "C" fn rust_SRCWRHTTPReq_body_add_file_handle(
	object: &mut SRCWRHTTPReq,
	file: &mut IFileObject,
) -> Option<NonZeroU32> {
	let mut reader = BufReader::with_capacity(4096, file);
	let _count = reader
		.read_to_end(object.body.get_or_insert_default())
		.ok()?;
	NonZeroU32::new(1)
}

///////////////////////////////////////////////////////////////////////////////////////

#[unsafe(no_mangle)]
pub extern "C" fn rust_SRCWRHTTPReq_local_address(
	object: &mut SRCWRHTTPReq,
	addr: *const c_char,
) -> Option<NonZeroU32> {
	object.local_address = Some(IpAddr::from_str(strxx(addr, true, 0)?).ok()?);
	NonZeroU32::new(1)
}

#[unsafe(no_mangle)]
pub extern "C" fn rust_SRCWRHTTPReq_basic_auth(
	object: &mut SRCWRHTTPReq,
	username: *const c_char,
	password: *const c_char,
) -> Option<NonZeroU32> {
	object.basic_auth = Some((
		strxx(username, false, 0)?.to_string(),
		strxx(password, false, 0).map(|s| s.to_string()),
	));
	NonZeroU32::new(1)
}

#[unsafe(no_mangle)]
pub extern "C" fn rust_SRCWRHTTPReq_allow_invalid_certs(object: &mut SRCWRHTTPReq) {
	object.allow_invalid_certs = true;
}

///////////////////////////////////////////////////////////////////////////////////////

#[unsafe(no_mangle)]
pub extern "C" fn rust_SRCWRHTTPReq_max_redirections_get(object: &mut SRCWRHTTPReq) -> u32 {
	object.max_redirections
}

#[unsafe(no_mangle)]
pub extern "C" fn rust_SRCWRHTTPReq_timeout_get(object: &mut SRCWRHTTPReq) -> u32 {
	object.timeout
}

#[unsafe(no_mangle)]
pub extern "C" fn rust_SRCWRHTTPReq_connect_timeout_get(object: &mut SRCWRHTTPReq) -> u32 {
	object.connect_timeout
}

#[unsafe(no_mangle)]
pub extern "C" fn rust_SRCWRHTTPReq_max_redirections_set(object: &mut SRCWRHTTPReq, value: u32) {
	object.max_redirections = value;
}

#[unsafe(no_mangle)]
pub extern "C" fn rust_SRCWRHTTPReq_timeout_set(object: &mut SRCWRHTTPReq, value: u32) {
	object.timeout = value;
}

#[unsafe(no_mangle)]
pub extern "C" fn rust_SRCWRHTTPReq_connect_timeout_set(object: &mut SRCWRHTTPReq, value: u32) {
	object.connect_timeout = value;
}

///////////////////////////////////////////////////////////////////////////////////////

fn resp_resolve_text(object: &mut SRCWRHTTPResp) {
	if object.text.is_none() {
		object.text = Some(reqwest_text_with_charset::yeah(
			&object.bytes,
			&object.headers,
		));
	}
}

#[unsafe(no_mangle)]
pub extern "C" fn rust_SRCWRHTTPResp_header(
	object: &mut SRCWRHTTPResp,
	name: *const c_char,
	buffer: *mut u8,
	maxlength: i32,
) -> Option<NonZeroU32> {
	if maxlength < 2 {
		return None; // FUCK YOU!
	}
	let name = strxx(name, false, 0)?;
	let buffer = unsafe { std::slice::from_raw_parts_mut(buffer, maxlength as usize) };
	let header = object.headers.get(name)?;
	let sz = core::cmp::min(header.len(), buffer.len() - 1);
	buffer[..sz].clone_from_slice(&header.as_bytes()[..sz]);
	buffer[sz] = b'\0';
	NonZeroU32::new(sz as u32)
}

#[unsafe(no_mangle)]
pub extern "C" fn rust_SRCWRHTTPResp_get(
	object: &mut SRCWRHTTPResp,
	buffer: *mut u8,
	maxlength: usize,
	flags: u32,
) -> Option<core::num::NonZeroUsize> {
	if maxlength < 2 {
		return None;
	}
	const H_AS_BYTES: u32 = 1 << 0;
	let s = if 0 != (flags & H_AS_BYTES) {
		&object.bytes
	} else {
		resp_resolve_text(object);
		if let Some(t) = object.text.as_ref() {
			t.as_bytes()
		} else {
			unreachable!()
		}
	};
	write_to_sp_buf(s, None, buffer, 0, maxlength, 0)
}

#[unsafe(no_mangle)]
pub extern "C" fn rust_SRCWRHTTPResp_status_get(object: &mut SRCWRHTTPResp) -> i32 {
	object.status
}

#[unsafe(no_mangle)]
pub extern "C" fn rust_SRCWRHTTPResp_text_length_get(object: &mut SRCWRHTTPResp) -> usize {
	resp_resolve_text(object);
	object.text.as_ref().unwrap().len()
}

#[unsafe(no_mangle)]
pub extern "C" fn rust_SRCWRHTTPResp_byte_length_get(object: &mut SRCWRHTTPResp) -> usize {
	object.bytes.len()
}

#[unsafe(no_mangle)]
pub extern "C" fn rust_SRCWRHTTPResp_json_get_inner(
	object: &mut SRCWRHTTPResp,
	outlen: &mut usize,
) -> *const u8 {
	resp_resolve_text(object);
	if let Some(t) = object.text.as_ref() {
		*outlen = t.len();
		t.as_ptr()
	} else {
		unreachable!()
	}
}

#[unsafe(no_mangle)]
pub extern "C" fn rust_SRCWRWebsocketMsg_get(
	object: &mut SRCWRWebsocketMsg,
	buffer: *mut u8,
	maxlength: usize,
	_flags: u32,
) -> Option<core::num::NonZeroUsize> {
	if maxlength < 2 {
		return None;
	}
	write_to_sp_buf(object.text.as_bytes(), None, buffer, 0, maxlength, 0)
}

#[unsafe(no_mangle)]
pub extern "C" fn rust_SRCWRWebsocketMsg_length_get(object: &mut SRCWRWebsocketMsg) -> usize {
	object.text.len()
}

#[unsafe(no_mangle)]
pub extern "C" fn rust_SRCWRWebsocketMsg_json_get_inner(
	object: &mut SRCWRWebsocketMsg,
	outlen: &mut usize,
) -> *const u8 {
	*outlen = object.text.len();
	object.text.as_ptr()
}
