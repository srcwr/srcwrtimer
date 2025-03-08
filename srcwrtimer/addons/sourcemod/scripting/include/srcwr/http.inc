// SPDX-License-Identifier: GPL-3.0-or-later
// Copyright 2022 rtldg <rtldg@protonmail.com>
// This file is part of srcwrtimer (https://github.com/srcwr/srcwrtimer/)

#if defined _srcwr_http_included
	#endinput
#endif
#define _srcwr_http_included
#pragma semicolon 1


#define H_AS_BYTES (1 << 0)


typeset SRCWRHTTPCallback {
	function void(any value, const char[] error, SRCWRHTTPResp resp);
}

typeset SRCWRHTTPDownloadCallback {
	function void(any value, const char[] error, const char[] filename);
}

// TODO: AppendFormParam & PostForm like ripext?

methodmap SRCWRHTTPReq < Handle {
	// You can supply varargs to be used in formatting if you want...
	public native SRCWRHTTPReq(const char[] url_format, any ...);

	// Sends the request :)
	// Closes this handle too.
	// If `.method("SOMETHING")` hasn't been called then `method` defaults to GET. This is just a shorter way to set method...
	public native void YEET(SRCWRHTTPCallback callback, any value = 0, const char[] method = "");

	// Downloads the response to a file.
	// Closes this handle too.
	// Will only write file if the status code is a success (200-299).
	// TODO: Add flag to still save to file even on a non-success status.
	public native void Download(SRCWRHTTPDownloadCallback callback, any value, const char[] path_format, any ...);

	// file upload might want `Content-Type: application/octet-stream`...
	// JSON APIs usually want `Content-Type: application/json`...

	// GET, POST, PUT, PATCH, DELETE, HEAD
	// Defaults to GET
	public native bool method(const char[] method);
	//public native void version(const char[] version);
	public native bool header(const char[] key, const char[] value, any ...);
	// public native void formparam(const char[] name, const char[] value, any ...);
	public native bool query_param(const char[] key, const char[] value, any ...);

	// Can be used to clear the body with `body_set(0, "");`
	// Can also format: `body_set(-1, "Hello %d", 123);` (this does `Hello 123`)
	// Note: a `length` of `0` will use `strlen(content)`.
	public native void body_set(int length=0, const char[] content, any ...);
	// Adds the file's contents to the body (in the request-sending thread)
	public native void body_add_file_on_send(const char[] path_format, any ...);
	// Adds content to the body (immediately)
	public native void body_add(int length=0, const char[] content, any ...);
	// Adds the file's contents to the body (immediately)
	public native void body_add_file(const char[] path_format, any ...);
	public native void body_add_file_handle(File file);
#if defined _srcwr_json_included
	// Adds the JSON object to the request body...
	public native void body_add_json(SRCWRJSON json);
#endif

	// TODO: form_param & postform & all the nice reqwest multipart stuff...

	// Local address to bind the sending socket to.
	//  The default behavior is to use the IP from the
	//  server's commandline (the "+ip 1.2.3.4" | "-ip 1.2.3.4").
	//  If that's not set, then it'll just use whatever IP the OS defaults to.
	// addr="" will use whatever the OS defaults to.
	public native void local_address(const char[] addr);
	// Sets the HTTP Basic Auth username & password.....
	public native void basic_auth(const char[] username, const char[] password);
	// ClientBuilder.danger_accept_invalid_certs
	public native void allow_invalid_certs();

	// TODO: cookie_provider & cookie_store

#if 0
	property int max_recv_speed {
		public native get();
		public native set(int speed);
	}
	property int max_send_speed {
		public native get();
		public native set(int speed);
	}
#endif

	// Default is 10.
	property int max_redirections {
		public native get();
		public native set(int redirections);
	}
	// The timeout is applied from when the request starts connecting until the response body has finished.
	// Default is 30000 milliseconds (30 seconds).
	property int timeout {
		public native get();
		public native set(int milliseconds);
	}
	// Set a timeout for only the connect phase.
	// Default is 10000 milliseconds (10 seconds).
	property int connect_timeout {
		public native get();
		public native set(int milliseconds);
	}
}

methodmap SRCWRHTTPResp < Handle {
	// Returns the number of chars written to the buffer.
	// Returns 0 if the header is missing...
	public native int header(const char[] name, char[] buffer, int maxlength);

	// TODO: Split int get_bytes & get_text?
	// TODO: Let you get from offsets in the file?
	// Returns the number of chars written.
	// Will use UTF-8 by default & NUL-terminate the string.
	// You can also pass H_AS_BYTES in `flags` (still NUL-terminates the buffer).
	public native int get(char[] buf, int buflen, int flags=0);

	// TODO: Add get_url?
	// TODO: Add an option to save-to-file (that's separate from the request.Download() function...)
	//       Maybe you need headers and such to determine whether to save it.
	// TODO: Add a GetLines() or something that fills an ArrayList?. Would be very epic... esp for GetMap
	//       Also a GetLine() that only reads a single line but then we have to store a Lines iterator...
	//       We could look into that srcwrutil SmolStringList thing too (since ArrayLists are fat)

	// HTTP status code
	property int status {
		public native get();
	}

	// Returns the number of bytes needed to hold a UTF-8 string of the response body. (does not include the NUL-terminator)
	property int text_length {
		public native get();
	}

	// Returns the number of bytes in the response body.
	property int byte_length {
		public native get();
	}

#if defined _srcwr_json_included
	property SRCWRJSON json {
		public native get();
	}
#endif
}

typeset SRCWRWebsocketConnectionCallback {
	function void(any data, SRCWRWebsocket ws, const char[] close_reason);
}

typeset SRCWRWebsocketMsgCallback {
	function void(any data, SRCWRWebsocket ws, SRCWRWebsocketMsg msg);
}

// TODO: Figure out how to make local_address for websockets...
//       Most of these libraries don't have an easy way to do it.
//       I think I'd need to make a tcp-connector or something...
methodmap SRCWRWebsocket < Handle {
	public native SRCWRWebsocket();
	public native bool header(const char[] key, const char[] value, any ...);
#if defined _srcwr_json_included
	public native bool write_json(SRCWRJSON json);
#endif
	public native bool write_str(const char[] content, any ...);
	public native bool YEET(
		  SRCWRWebsocketConnectionCallback connectcb
		, SRCWRWebsocketMsgCallback msgcb
		, any data
		, const char[] url_format
		, any ...
	);
}

methodmap SRCWRWebsocketMsg < Handle {
	// Returns the number of chars written.
	public native int get(char[] buf, int buflen, int flags=0);

	// Returns the number of bytes needed to hold a UTF-8 string of the response body. (does not include the NUL-terminator)
	property int length {
		public native get();
	}

#if defined _srcwr_json_included
	property SRCWRJSON json {
		public native get();
	}
#endif
}



public int srcwrhttp_compat_version = 1;


public Extension __ext_srcwrhttp = {
	name = "srcwrhttp",
	file = "srcwrhttp.ext",
#if defined AUTOLOAD_EXTENSIONS
	autoload = 1,
#else
	autoload = 0,
#endif
#if defined REQUIRE_EXTENSIONS
	required = 1,
#else
	required = 0,
#endif
};

#if !defined REQUIRE_EXTENSIONS
public void __ext_srcwrhttp_SetNTVOptional()
{
	MarkNativeAsOptional("SRCWRHTTPReq.SRCWRHTTPReq");

	MarkNativeAsOptional("SRCWRHTTPReq.YEET");
	MarkNativeAsOptional("SRCWRHTTPReq.Download");

	MarkNativeAsOptional("SRCWRHTTPReq.method");
	MarkNativeAsOptional("SRCWRHTTPReq.header");
	MarkNativeAsOptional("SRCWRHTTPReq.query_param");

	MarkNativeAsOptional("SRCWRHTTPReq.body_set");
	MarkNativeAsOptional("SRCWRHTTPReq.body_add_file_on_send");
	MarkNativeAsOptional("SRCWRHTTPReq.body_add");
	MarkNativeAsOptional("SRCWRHTTPReq.body_add_file");
	MarkNativeAsOptional("SRCWRHTTPReq.body_add_file_handle");

	MarkNativeAsOptional("SRCWRHTTPReq.local_address");
	MarkNativeAsOptional("SRCWRHTTPReq.basic_auth");
	MarkNativeAsOptional("SRCWRHTTPReq.allow_invalid_certs");

	MarkNativeAsOptional("SRCWRHTTPReq.max_redirections.get");
	MarkNativeAsOptional("SRCWRHTTPReq.max_redirections.set");
	MarkNativeAsOptional("SRCWRHTTPReq.timeout.get");
	MarkNativeAsOptional("SRCWRHTTPReq.timeout.set");
	MarkNativeAsOptional("SRCWRHTTPReq.connect_timeout.get");
	MarkNativeAsOptional("SRCWRHTTPReq.connect_timeout.set");

	MarkNativeAsOptional("SRCWRHTTPResp.header");
	MarkNativeAsOptional("SRCWRHTTPResp.get");
	MarkNativeAsOptional("SRCWRHTTPResp.status.get");
	MarkNativeAsOptional("SRCWRHTTPResp.text_length.get");
	MarkNativeAsOptional("SRCWRHTTPResp.byte_length.get");
	MarkNativeAsOptional("SRCWRHTTPResp.json.get");

	MarkNativeAsOptional("SRCWRWebsocket.SRCWRWebsocket");
	MarkNativeAsOptional("SRCWRWebsocket.header");
	MarkNativeAsOptional("SRCWRWebsocket.write_json");
	MarkNativeAsOptional("SRCWRWebsocket.write_str");
	MarkNativeAsOptional("SRCWRWebsocket.YEET");

	MarkNativeAsOptional("SRCWRWebsocketMsg.get");
	MarkNativeAsOptional("SRCWRWebsocketMsg.length.get");
	MarkNativeAsOptional("SRCWRWebsocketMsg.json.get");
}
#endif
