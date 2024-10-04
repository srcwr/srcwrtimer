// SPDX-License-Identifier: GPL-3.0-or-later
// Copyright 2022 rtldg <rtldg@protonmail.com>
// This file is part of srcwrtimer (https://github.com/srcwr/srcwrtimer/)

#![allow(non_snake_case)]

mod gitstuff;

use core::ffi::c_char;
use core::ffi::c_void;
use std::ffi::CStr;
use std::ffi::OsString;
use std::string::String;

use lazy_static::lazy_static;
use parking_lot::Condvar;
use parking_lot::Mutex;

extshared::smext_conf_boilerplate_extension_info!();

enum SyncType {
	Clone(String), // url
	Pull(String),  // branch I guess...
}
struct SyncIn {
	asdf:     SyncType,
	path:     OsString,
	data:     i32,
	callback: *const c_void,
}
unsafe impl Send for SyncIn {} // so we can store the pointers...

struct SyncOut {
	data:     i32,
	callback: *const c_void,
	err:      Option<String>,
}
unsafe impl Send for SyncOut {} // so we can store the pointers...

static mut UTTTT: Condvar = Condvar::new();
lazy_static! {
	static ref INQUEUE: Mutex<(bool, Vec<SyncIn>)> = Mutex::new((false, vec![]));
	static ref OUTQUEUE: Mutex<Vec<SyncOut>> = Mutex::new(vec![]);
}

static mut THREAD: Option<std::thread::JoinHandle<()>> = None;

#[no_mangle]
pub fn rust_sdk_on_load(late: bool) -> Result<(), Box<dyn std::error::Error>> {
	unsafe {
		//git2_curl::register(curl::easy::Easy::new());

		let https = hyper_rustls::HttpsConnectorBuilder::new()
			.with_webpki_roots()
			.https_or_http()
			.enable_http2()
			.build();

		git2_hyper::register(
			hyper::Client::builder()
				.http1_title_case_headers(true)
				.build(https),
		);
	}

	let thread = std::thread::Builder::new()
		.name(concat!(env!("CARGO_PKG_NAME", " worker")).to_string())
		.spawn(|| {
			syncer_thread();
		})?;

	unsafe {
		THREAD = Some(thread);
		extshared::cpp_add_game_frame_hook(game_frame_syncer);
	}

	Ok(())
}

#[no_mangle]
pub extern "C" fn rust_sdk_on_unload() {
	unsafe {
		extshared::cpp_remove_game_frame_hook(game_frame_syncer);

		{
			let mut inqueue = INQUEUE.lock();
			inqueue.0 = true; // please die
		}
		UTTTT.notify_one();

		println!(
			"[{}] Trying to kill Syncer thread...",
			env!("CARGO_PKG_NAME")
		);
		THREAD
			.take()
			.expect("Failed to take thread handle")
			.join()
			.unwrap();

		// git2::transport::unregister("http");
		// git2::transport::unregister("https");
	}
}

#[no_mangle]
pub extern "C" fn rust_sdk_on_unload() {}

#[no_mangle]
pub extern "C" fn rust_on_core_map_start(
	_edict_list: *mut c_void,
	_edict_count: i32,
	_client_max: i32,
) {
}

#[no_mangle]
pub extern "C" fn rust_on_core_map_end() {}

extern "C" fn game_frame_syncer(_simulating: bool) {
	let mut items = {
		let mut outqueue = OUTQUEUE.lock();
		if outqueue.is_empty() {
			return;
		}
		std::mem::replace(&mut *outqueue, vec![])
	};
	for item in items.iter_mut() {
		// println!("calling item. data = {} | err = {:?}", item.data, item.err);
		let _ = call_item(item);
	}
}

fn syncer_thread() {
	loop {
		let items = {
			let mut inqueue = INQUEUE.lock();
			unsafe { UTTTT.wait(&mut inqueue) };
			// please die
			if inqueue.0 == true {
				return;
			}
			std::mem::replace(&mut inqueue.1, vec![])
		};

		for item in items {
			let err = handle_item(&item).err();
			// println!("handled item. data = {} | err = {:?}", item.data, err);
			OUTQUEUE.lock().push(SyncOut {
				data:     item.data,
				callback: item.callback,
				err:      err,
			});
		}
	}
}

#[no_mangle]
pub extern "C" fn rust_SRCWR_Syncer_action(
	thing: *const c_char,
	path: *const c_char,
	forward: *const c_void,
	data: i32,
	is_clone_else_pull: bool,
) {
	// TODO: remove unwrap
	let thing = unsafe { CStr::from_ptr(thing) }
		.to_str()
		.unwrap()
		.to_owned();
	let thing = match is_clone_else_pull {
		true => SyncType::Clone(thing),
		false => SyncType::Pull(thing),
	};
	// TODO: remove unwrap
	let path = OsString::from(unsafe { CStr::from_ptr(path) }.to_str().unwrap());

	{
		let mut inqueue = INQUEUE.lock();
		inqueue.1.push(SyncIn {
			asdf:     thing,
			path:     path,
			data:     data,
			callback: forward,
		});
	}

	unsafe {
		UTTTT.notify_one();
	}
}

fn call_item(item: SyncOut) -> Option<i32> {
	unsafe {
		extshared::cpp_forward_push_cell(item.callback, item.data);
		extshared::cpp_forward_push_string(
			item.callback,
			if let Some(err) = &item.err {
				err.as_ptr()
			} else {
				"\0".as_ptr()
			},
		);
		extshared::cpp_forward_execute(item.callback, std::ptr::null_mut());
		extshared::cpp_forward_release(item.callback);
	}
	Some(1)
}

fn handle_item(item: &SyncIn) -> Result<(), String> {
	if let SyncType::Clone(url) = &item.asdf {
		let err = git2::Repository::clone(&url, &item.path)
			.map_err(|e| format!("failed to clone '{}' to '{:?}': {}\0", &url, &item.path, e))?;
	} else if let SyncType::Pull(remote_branch) = &item.asdf {
		// pull
		let repo = git2::Repository::open(&item.path)
			.map_err(|e| format!("failed to open '{:?}': {}\0", &item.path, e))?;
		let remote_name = "origin";
		let mut remote = repo.find_remote(remote_name).map_err(|e| {
			format!(
				"failed to find {:?} remote {}: {}\0",
				&item.path, remote_name, e
			)
		})?;
		let fetch_commit = gitstuff::do_fetch(&repo, &[remote_branch], &mut remote).map_err(|e| {
			format!(
				"failed to fetch '{:?}' branch '{}': {}\0",
				&item.path, remote_branch, e
			)
		})?;
		// println!("fetch_commit = {:?}", fetch_commit.id());
		gitstuff::do_merge(&repo, remote_branch, fetch_commit).map_err(|e| {
			format!(
				"failed to fast forward '{:?}' branch '{}': {}\0",
				&item.path, remote_branch, e
			)
		})?;
	}
	Ok(())
}
