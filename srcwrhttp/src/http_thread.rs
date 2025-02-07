// SPDX-License-Identifier: GPL-3.0-or-later
// Copyright 2022-2024 rtldg <rtldg@protonmail.com>
// This file is part of srcwrtimer (https://github.com/srcwr/srcwrtimer/)

// TODO: Explain everything LOL

// TODO: Bleh, static muts...
#![allow(static_mut_refs)]

use std::collections::HashMap;
use std::ffi::c_char;
use std::fmt::Write;
use std::num::NonZeroU32;
use std::str::FromStr;

use anyhow::anyhow;
use bytes::BytesMut;
use extshared::*;
use futures::SinkExt;
use futures::StreamExt;
use tokio::io::AsyncReadExt;
use tokio::io::AsyncWriteExt;
use tokio::io::BufWriter;
use tokio::sync::mpsc;
use tokio::sync::mpsc::UnboundedReceiver;
use tokio::sync::mpsc::UnboundedSender;
use tokio_tungstenite::tungstenite;
use tokio_tungstenite::tungstenite::client::IntoClientRequest;

use crate::*;

static mut TO_ASYNC: Option<UnboundedSender<ToAsyncThread>> = None;
static mut ASYNC_THREAD: Option<std::thread::JoinHandle<()>> = None;

static mut FROM_ASYNC: Option<crossbeam::channel::Receiver<ForSpForward>> = None;
static mut TO_SP: Option<crossbeam::channel::Sender<ForSpForward>> = None;

static mut WSSTREAMS: Option<HashMap<u32, SRCWRWebsocket>> = None;

enum ForSpForward {
	WsMsg(SRCWRWebsocketMsg),
	HttpResp(SRCWRHTTPResp),
}

#[unsafe(no_mangle)]
pub extern "C" fn rust_handle_destroy_SRCWRWebsocket(streamid: u32) {
	if let Some(object) = unsafe { WSSTREAMS.as_mut().unwrap().remove(&streamid) } {
		// println!("We need to delete {:?}", object);
		if let WsInner::Writer(w) = &object.inner {
			if object.connection_forward.is_none() {
				do_forward_ws(&object, SRCWRWebsocketMsg {
					streamid: object.streamid,
					state:    WsState::Closed,
					text:     "handled deleted".into(),
				});
			}
			// Tell ws_thread to exit...
			let _ = w.send(SRCWRWebsocketMsg {
				streamid: object.streamid,
				state:    WsState::Closed,
				text:     "".into(),
			});
		}
		unsafe {
			if let Some(fw) = object.connection_forward {
				cpp_forward_release(fw);
			}
			if let Some(fw) = object.message_forward {
				cpp_forward_release(fw);
			}
		}
		drop(object);
	}
}

#[unsafe(no_mangle)]
pub extern "C" fn rust_handle_size_SRCWRWebsocket(_streamid: u32, size: &mut u32) -> bool {
	// there's no nice way to calculate this so we'll just use a placeholder that's greater than size_of<WebsocketStream<MaybeTlsStream>>
	*size = size_of::<SRCWRWebsocket>() as u32 + 512;
	true
}

#[unsafe(no_mangle)]
pub extern "C" fn rust_SRCWRWebsocket_SRCWRWebsocket() -> u32 {
	let streamid = unsafe {
		static mut LAST_STREAMID: u32 = 0;
		LAST_STREAMID += 1;
		LAST_STREAMID
	};
	let _ = unsafe {
		WSSTREAMS.as_mut().unwrap().insert(streamid, SRCWRWebsocket {
			streamid,
			handle: 0,
			inner: WsInner::Request(Default::default()),
			user_value: 0,
			connection_forward: None,
			message_forward: None,
		})
	};
	streamid
}

#[unsafe(no_mangle)]
pub extern "C" fn rust_SRCWRWebsocket_set_handle(streamid: u32, handle: u32) {
	let object = unsafe { WSSTREAMS.as_mut().unwrap().get_mut(&streamid).unwrap() };
	object.handle = handle;
}

#[unsafe(no_mangle)]
pub extern "C" fn rust_SRCWRWebsocket_header(streamid: u32, key: *const c_char, value: *const c_char) -> Option<NonZeroU32> {
	let object = unsafe { WSSTREAMS.as_mut().unwrap().get_mut(&streamid).unwrap() };
	if let WsInner::Request(req) = &mut object.inner {
		let key = strxx(key, false, 0)?;
		let value = strxx(value, false, 0)?;
		req.headers_mut().insert(key, http::HeaderValue::from_str(value).ok()?)?;
		unsafe { Some(NonZeroU32::new_unchecked(1)) }
	} else {
		None
	}
}

#[unsafe(no_mangle)]
pub extern "C" fn rust_SRCWRWebsocket_write_json(streamid: u32, v: &mut serde_json::Value) {
	let mut buf = BytesMut::new();
	write!(&mut buf, "{}", v).unwrap();
	rust_SRCWRWebsocket_write_str_inner(streamid, Utf8Bytes::try_from(buf.freeze()).unwrap());
}

#[unsafe(no_mangle)]
pub extern "C" fn rust_SRCWRWebsocket_write_str(streamid: u32, s: *const c_char) -> Option<NonZeroU32> {
	let s = strxx(s, false, 0)?;
	rust_SRCWRWebsocket_write_str_inner(streamid, s.into())
}

fn rust_SRCWRWebsocket_write_str_inner(streamid: u32, s: Utf8Bytes) -> Option<NonZeroU32> {
	let object = unsafe { WSSTREAMS.as_mut().unwrap().get_mut(&streamid).unwrap() };
	if let WsInner::Writer(w) = &mut object.inner {
		w.send(SRCWRWebsocketMsg {
			streamid,
			state: WsState::Msg,
			text: s,
		})
		.ok()?;
		None
	} else {
		None
	}
}

#[unsafe(no_mangle)]
pub extern "C" fn rust_SRCWRWebsocket_YEET(
	streamid: u32,
	url: *const c_char,
	user_value: u32,
	connection_forward: NonNull<c_void>,
	message_forward: NonNull<c_void>,
) -> Option<NonZeroU32> {
	let url = strxx(url, false, 0)?;
	let mut newreq = url.into_client_request().ok()?;
	let object = unsafe { WSSTREAMS.as_mut().unwrap().get_mut(&streamid).unwrap() };
	match &mut object.inner {
		WsInner::Writer(_) => return None,
		WsInner::Request(req) => {
			*req.uri_mut() = http::Uri::from_str(url).ok()?;
			object.user_value = user_value;
			object.connection_forward = Some(connection_forward);
			object.message_forward = Some(message_forward);
			let (to_ws, from_sp) = tokio::sync::mpsc::unbounded_channel();
			let req = std::mem::replace(&mut object.inner, WsInner::Writer(to_ws));
			if let WsInner::Request(req) = req {
				newreq.headers_mut().extend(req.headers().clone());
				send_to_request_builder_thread(ToAsyncThread::Ws((newreq, streamid, from_sp)));
				return unsafe { Some(NonZeroU32::new_unchecked(1)) };
			}
		},
	}
	None
}

#[unsafe(no_mangle)]
pub extern "C" fn rust_handle_destroy_SRCWRWebsocketMsg(object: *mut SRCWRWebsocketMsg) {
	let object = unsafe { Box::from_raw(object) };
	drop(object);
}

#[unsafe(no_mangle)]
pub extern "C" fn rust_handle_size_SRCWRWebsocketMsg(object: &mut SRCWRWebsocketMsg, size: &mut u32) -> bool {
	*size = (object.text.len() + size_of::<SRCWRWebsocketMsg>()) as u32;
	true
}

pub fn send_to_request_builder_thread(thing: ToAsyncThread) {
	unsafe {
		TO_ASYNC.as_ref().unwrap().send(thing).unwrap();
	}
}

async fn ws_thread(
	req: Request<()>,
	streamid: u32,
	mut from_sp: UnboundedReceiver<SRCWRWebsocketMsg>,
) -> anyhow::Result<(), anyhow::Error> {
	let (mut ws_stream, _) = tokio_tungstenite::connect_async(req).await?;
	unsafe {
		TO_SP.as_ref().unwrap().send(ForSpForward::WsMsg(SRCWRWebsocketMsg {
			streamid,
			state: WsState::Open,
			text: "".into(),
		}))?;
	}
	loop {
		tokio::select! {
			x = from_sp.recv() => {
				let x = x.ok_or(anyhow!("Failed to receive message from sourcepawn"))?;
				// println!("from_sp: {:?}", x);
				if x.state == WsState::Closed {
					let _ = ws_stream.close(Some(tungstenite::protocol::CloseFrame {
						code: tungstenite::protocol::frame::coding::CloseCode::Normal,
						reason: "".into(),
					})).await;
					return Ok(());
				}
				ws_stream.send(tungstenite::Message::Text(x.text.clone())).await?;
			}
			msg = ws_stream.next() => {
				let msg = msg.ok_or(anyhow!("Failed to read next WS message"))??;
				// println!("ws_stream: {:?}", msg);
				match msg {
					tungstenite::Message::Ping(v) => {
						ws_stream.send(tungstenite::Message::Pong(v)).await?;
					},
					tungstenite::Message::Pong(_) => {
						// whatever
					},
					tungstenite::Message::Close(cf) => {
						if let Some(cf) = cf {
							return Err(anyhow!("Code {} - {}", cf.code, cf.reason));
						} else {
							return Err(anyhow!("Unknown"));
						}
					},
					_ => {
						// Binary / Text
						if let Ok(text) = msg.into_text() {
							// println!("Sending text to SP: '{}'", text);
							let _ = unsafe {
								TO_SP.as_ref().unwrap().send(ForSpForward::WsMsg(
									SRCWRWebsocketMsg {
										streamid,
										state: WsState::Msg,
										text,
									}))
							};
						}
					}
				}
			}
		}
	}
}

async fn handle_req(client: reqwest::Client, mut req: SRCWRHTTPReq) -> SRCWRHTTPResp {
	let builder = client.request(req.method, req.url.unwrap());

	let builder = if let Some(x) = req.headers {
		builder.headers(x)
	} else {
		builder
	};

	let builder = if let Some(x) = req.basic_auth {
		builder.basic_auth(x.0, x.1)
	} else {
		builder
	};

	let builder = if let Some(filename) = req.body_file_on_send {
		let mut f = match tokio::fs::File::open(filename).await {
			Ok(f) => f,
			Err(e) => {
				return SRCWRHTTPResp {
					cbinfo: req.cbinfo,
					e: Some(e.to_string()),
					..Default::default()
				};
			},
		};

		// TODO: pass a stream instead of reading to the body...
		//       a combined stream of
		// combine streams:
		//   Body::wrap_stream(ReaderStream::new(file)) or something idk..
		if let Some(mut v) = req.body.take() {
			match f.read_to_end(&mut v).await {
				Ok(_) => (),
				Err(e) => {
					return SRCWRHTTPResp {
						cbinfo: req.cbinfo,
						e: Some(e.to_string()),
						..Default::default()
					};
				},
			};
			builder.body(v)
		} else {
			builder.body(f)
		}
	} else if let Some(v) = req.body {
		builder.body(v)
	} else {
		builder
	};

	match builder.send().await {
		Ok(resp) => {
			if let Some(download_path_real) = &req.cbinfo.download_path_real {
				// i'm probably cloning this fucking cbinfo string a shit ton... fuck it...
				return match download_handler(resp, download_path_real).await {
					Ok(mut shr) => {
						shr.cbinfo = req.cbinfo;
						shr
					},
					Err(e) => {
						// probably stream-reading error or file-writing error
						let _ = tokio::fs::remove_file(download_path_real).await;
						SRCWRHTTPResp {
							cbinfo: req.cbinfo,
							e: Some(e.to_string()),
							..Default::default()
						}
					},
				};
			}

			let status = resp.status().as_u16() as i32;
			let headers = resp.headers().clone();
			let bytes = resp.bytes().await.unwrap_or_default();

			SRCWRHTTPResp {
				headers,
				status,
				text: None,
				bytes,
				cbinfo: req.cbinfo,
				e: None,
			}
		},
		Err(e) => SRCWRHTTPResp {
			cbinfo: req.cbinfo,
			e: Some(e.to_string()),
			..Default::default()
		},
	}
}

async fn download_handler(resp: reqwest::Response, download_path: &str) -> anyhow::Result<SRCWRHTTPResp> {
	anyhow::ensure!(resp.status().is_success(), "Non-success status: {}", resp.status());

	let f = tokio::fs::File::create(download_path).await?;
	let mut bufwriter = BufWriter::new(f);

	let status = resp.status().as_u16() as i32;
	let headers = resp.headers().clone();
	let mut stream = resp.bytes_stream();

	let (tx, mut rx) = tokio::sync::mpsc::unbounded_channel::<Bytes>();
	let file_writer = tokio::spawn(async move {
		while let Some(chunk) = rx.recv().await {
			bufwriter.write_all(&chunk).await?;
		}
		bufwriter.into_inner().sync_all().await?;
		anyhow::Ok(())
	});
	while let Some(item) = stream.next().await {
		let chunk = item?;
		tx.send(chunk)?;
	}
	drop(tx);
	file_writer.await??;

	Ok(SRCWRHTTPResp {
		headers,
		status,
		..Default::default()
	})
}

async fn async_receiver(default_client: reqwest::Client, mut recv: UnboundedReceiver<ToAsyncThread>) {
	let mut tasks = vec![];
	while let Some(thing) = recv.recv().await {
		match thing {
			ToAsyncThread::Req(req) => {
				let client = default_client.clone();
				tasks.push(tokio::spawn(async move {
					// println!("recv_loop child spawned... waiting 10s");
					// tokio::time::sleep(tokio::time::Duration::from_secs(10)).await;
					// println!("10s is up!");
					let resp = handle_req(client, req).await;
					unsafe {
						let _ = TO_SP.as_ref().unwrap().send(ForSpForward::HttpResp(resp));
					}
				}));
			},
			ToAsyncThread::Ws((req, streamid, from_sp)) => {
				tasks.push(tokio::spawn(async move {
					if let Err(close_reason) = ws_thread(req, streamid, from_sp).await {
						let _ = unsafe {
							let mut close_reason_buf = BytesMut::new();
							write!(&mut close_reason_buf, "{}", close_reason).unwrap();
							let close_reason = Utf8Bytes::try_from(close_reason_buf.freeze()).unwrap();
							TO_SP.as_ref().unwrap().send(ForSpForward::WsMsg(SRCWRWebsocketMsg {
								streamid,
								state: WsState::Closed,
								text: close_reason,
							}))
						};
						// println!("Leaving ws_thread?");
					}
				}));
			},
		}
		// remove finished tasks where possible... because I don't want
		// `tasks` to be full of 300 tasks to join_all on...
		tasks.retain(|task| !task.is_finished());
	}
	if !tasks.is_empty() {
		println!(">>> srcwrhttp: waiting for requests to finish...");
		let _ = futures::future::join_all(tasks).await;
	}
	// println!("exiting recv_loop...");
}

pub fn http_thread_load() -> Result<(), Box<dyn std::error::Error>> {
	unsafe {
		WSSTREAMS = Some(Default::default());
	}

	// TODO: Fake this useragent shit?
	// something like "srcwrhttp/1.0.0 (+https://github.com/srcwr/srcwrtimer/srcwrhttp)"
	let useragent = format!(
		"{}/{} (+{}/{})",
		env!("CARGO_PKG_NAME"),
		env!("CARGO_PKG_VERSION"),
		env!("CARGO_PKG_REPOSITORY"),
		env!("CARGO_PKG_NAME")
	);

	let default_client = reqwest::ClientBuilder::new()
		.user_agent(useragent)
		.local_address(server_ip::commandline_bindip())
		.connect_timeout(core::time::Duration::from_secs(10))
		.timeout(core::time::Duration::from_secs(30))
		.build()?;

	let rt = tokio::runtime::Builder::new_multi_thread()
		.enable_all()
		.worker_threads(3) // I don't want to see 12 fucking threads lmao...
		.thread_name("SRCWRHTTP Tokio")
		.build()?;

	let (to_async, async_recv) = mpsc::unbounded_channel();

	let thread = std::thread::Builder::new().name("SRCWRHTTP Spawner".into()).spawn(move || {
		rt.block_on(async move {
			async_receiver(default_client, async_recv).await;
		});
		//println!("our block on is over...");
		drop(rt); /* this kills the thread pool */
		//println!("rt dropped");
	})?;

	let (to_sp, from_async) = crossbeam::channel::unbounded();

	unsafe {
		TO_SP = Some(to_sp);
		FROM_ASYNC = Some(from_async);
		TO_ASYNC = Some(to_async);
		ASYNC_THREAD = Some(thread);
		extshared::cpp_add_game_frame_hook(http_frame);
	}

	Ok(())
}

pub fn http_thread_unload() {
	unsafe {
		extshared::cpp_remove_game_frame_hook(http_frame);
	}

	unsafe {
		// I thought DestroyTypes() would've killed all the handles but apparently not....
		let mut handles: Vec<u32> = vec![];
		for obj in WSSTREAMS.as_mut().unwrap().values_mut() {
			handles.push(obj.handle);
		}
		// We collected to delete outside the WSSTREAMS loop because we would probably clobber the hashmap...
		for handle in handles {
			cpp_free_handle(handle);
		}

		TO_ASYNC = None; // drops sender & causes recv_loop to end
		ASYNC_THREAD.take().unwrap().join().unwrap(); // may take a moment...
		TO_SP = None;
	}

	// I want to drain all responses so all callbacks can delete `user_value` if it's a handle...
	let receiver = unsafe { FROM_ASYNC.take().unwrap() };
	while let Ok(x) = receiver.recv() {
		match x {
			ForSpForward::HttpResp(x) => do_forward(x),
			ForSpForward::WsMsg(_) => (),
		}
	}
}

// messy :(((
fn do_forward(mut resp: SRCWRHTTPResp) {
	// value
	// error
	// handle || filepath

	let fw = resp.cbinfo.forward.unwrap();
	let user_value = resp.cbinfo.user_value;

	let e = if resp.e.is_some() {
		let s = resp.e.as_mut().unwrap();
		s.push('\0');
		s.as_ptr()
	} else {
		// fucky lint
		#[allow(clippy::manual_c_str_literals)]
		"\0".as_ptr()
	};

	// take() so it doesn't get magically destroyed by cpp_forward_http_resp + FreeHandle...
	let download_path_sp = if let Some(mut s) = resp.cbinfo.download_path_sp.take() {
		s.push('\0');
		Some(s)
	} else {
		None
	};

	let download_path_ptr = download_path_sp.as_ref().map(|s| s.as_ptr()).unwrap_or(core::ptr::null());

	unsafe extern "C" {
		#[allow(improper_ctypes)]
		fn cpp_forward_http_resp(
			fw: NonNull<c_void>,
			user_value: i32,
			e: *const u8,
			object: *mut SRCWRHTTPResp,
			download_path: *const u8,
		);
	}

	unsafe {
		cpp_forward_http_resp(fw, user_value, e, Box::into_raw(Box::new(resp)), download_path_ptr);
	}

	// unsafe {
	//   cpp_forward_push_cell(fw, resp.cbinfo.user_value as i32);
	//   cpp_forward_push_string(fw, e);
	//   if let Some(s) = download_path {
	//     cpp_forward_push_string(fw, s);
	//   } else {
	//     cpp_forward_push_cell(fw, handle);
	//   }
	//   cpp_forward_execute(fw, &mut 0);
	//   cpp_forward_release(fw);
	// }
}

#[allow(unused_mut)]
fn do_forward_ws(ws: &SRCWRWebsocket, mut msg: SRCWRWebsocketMsg) {
	unsafe extern "C" {
		#[allow(improper_ctypes)]
		fn cpp_forward_websocket_msg(
			fw: NonNull<c_void>,
			wshandle: u32,
			user_value: u32,
			close_reason: *const u8,
			object: *mut SRCWRWebsocketMsg,
		);
	}
	// msg.text.push('\0');

	let mut close_reason = core::ptr::null();
	let fw = match msg.state {
		WsState::Msg => ws.message_forward,
		WsState::Open => ws.connection_forward,
		WsState::Closed => {
			close_reason = msg.text.as_ptr();
			ws.connection_forward
		},
	}
	.unwrap();

	unsafe {
		cpp_forward_websocket_msg(
			fw,
			ws.handle,
			ws.user_value,
			close_reason,
			if msg.state == WsState::Msg {
				Box::into_raw(Box::new(msg))
			} else {
				core::ptr::null_mut()
			},
		);
	}
}

// return true to process another message...
fn handle_from_async(resp: ForSpForward) -> bool {
	match resp {
		ForSpForward::HttpResp(x) => {
			do_forward(x);
			false
		},
		ForSpForward::WsMsg(msg) => {
			let streams = unsafe { WSSTREAMS.as_mut().unwrap() };
			let streamid = msg.streamid;
			let state = msg.state;
			if let Some(ws) = streams.get_mut(&streamid) {
				do_forward_ws(ws, msg);
				if state == WsState::Closed {
					let x = streams.remove(&streamid).unwrap();
					unsafe {
						if let Some(fw) = x.connection_forward {
							cpp_forward_release(fw);
						}
						if let Some(fw) = x.message_forward {
							cpp_forward_release(fw);
						}
						cpp_free_handle(x.handle);
					}
				}
				return false;
			}
			true
		},
	}
}

pub extern "C" fn http_frame(_simulating: bool) {
	unsafe {
		while let Ok(resp) = FROM_ASYNC.as_ref().unwrap().try_recv() {
			if !handle_from_async(resp) {
				break;
			}
		}
	}
}
