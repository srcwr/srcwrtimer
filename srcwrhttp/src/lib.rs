// SPDX-License-Identifier: GPL-3.0-or-later
// Copyright 2022 rtldg <rtldg@protonmail.com>
// This file is part of srcwrtimer (https://github.com/srcwr/srcwrtimer/)

#![allow(non_snake_case)]

mod http_thread;
mod ip_addr;
mod natives_http;
mod reqwest_text_with_charset;
mod server_ip;

use core::ffi::c_void;
use std::ptr::NonNull;

use bytes::Bytes;
use http::Request;
use mimalloc::MiMalloc;
use reqwest::header::HeaderMap;
use tokio_tungstenite::tungstenite::Utf8Bytes;

#[global_allocator]
static GLOBAL: MiMalloc = MiMalloc;

extshared::smext_conf_boilerplate_extension_info!();

#[derive(Debug, Default, Clone)]
pub struct CallbackInfo {
	user_value:         i32,
	forward:            Option<NonNull<c_void>>,
	download_path_sp:   Option<String>,
	download_path_real: Option<String>,
}
unsafe impl Send for CallbackInfo {} // so we can store the pointers...
unsafe impl Sync for CallbackInfo {} // so we can send this in crossbeam things...

#[derive(Debug, Default)]
pub struct SRCWRHTTPReq {
	url:     Option<reqwest::Url>,
	method:  reqwest::Method,
	headers: Option<reqwest::header::HeaderMap>,

	body:              Option<Vec<u8>>,
	body_file_on_send: Option<String>,

	local_address:       Option<std::net::IpAddr>, // requires new Client
	basic_auth:          Option<(String, Option<String>)>,
	allow_invalid_certs: bool, // requires new Client

	max_redirections: u32, // requires new Client
	timeout:          u32,
	connect_timeout:  u32, // requires new Client

	cbinfo: CallbackInfo,
}

#[derive(Debug, Default)]
pub struct SRCWRHTTPResp {
	headers: HeaderMap,
	status:  i32,
	text:    Option<String>,
	bytes:   Bytes,
	cbinfo:  CallbackInfo,
	e:       Option<String>,
}

#[derive(Debug)]
pub enum WsInner {
	Writer(tokio::sync::mpsc::UnboundedSender<SRCWRWebsocketMsg>),
	Request(tokio_tungstenite::tungstenite::http::Request<()>),
}

#[derive(Debug)]
pub struct SRCWRWebsocket {
	streamid:           u32,
	handle:             u32,
	inner:              WsInner,
	user_value:         u32,
	connection_forward: Option<NonNull<c_void>>,
	message_forward:    Option<NonNull<c_void>>,
}
unsafe impl Send for SRCWRWebsocket {} // so we can store the pointers...
unsafe impl Sync for SRCWRWebsocket {} // so we can send this in crossbeam things...

#[derive(Debug, PartialEq, Eq, Clone, Copy)]
pub enum WsState {
	Open,
	Closed,
	Msg,
}

#[derive(Debug)]
pub struct SRCWRWebsocketMsg {
	streamid: u32,
	state:    WsState,
	text:     Utf8Bytes,
}

#[derive(Debug)]
pub enum ToAsyncThread {
	Req(SRCWRHTTPReq),
	Ws((Request<()>, u32, tokio::sync::mpsc::UnboundedReceiver<SRCWRWebsocketMsg>)),
}

#[unsafe(no_mangle)]
pub fn rust_sdk_on_load(_late: bool) -> Result<(), Box<dyn std::error::Error>> {
	http_thread::http_thread_load()?;
	Ok(())
}

#[unsafe(no_mangle)]
pub extern "C" fn rust_sdk_on_unload() {
	http_thread::http_thread_unload();
}

#[unsafe(no_mangle)]
pub extern "C" fn rust_sdk_on_all_loaded() {}

#[unsafe(no_mangle)]
pub extern "C" fn rust_on_core_map_start(_edict_list: *mut core::ffi::c_void, _edict_count: i32, _client_max: i32) {}

#[unsafe(no_mangle)]
pub extern "C" fn rust_on_core_map_end() {}
