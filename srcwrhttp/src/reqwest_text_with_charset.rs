// SPDX-License-Identifier: MIT OR Apache-2.0
// Copyright 2017-2023 reqwest contributors
// Copied & trimmed from https://github.com/seanmonstar/reqwest/blob/b9d62a0323d96f11672a61a17bf8849baec00275/src/async_impl/response.rs#L206

use bytes::Bytes;
use encoding_rs::Encoding;
use encoding_rs::UTF_8;
use mime::Mime;
use reqwest::header::HeaderMap;

pub fn yeah(bytes: &Bytes, headers: &HeaderMap) -> String {
	let content_type = headers
		.get(reqwest::header::CONTENT_TYPE)
		.and_then(|value| value.to_str().ok())
		.and_then(|value| value.parse::<Mime>().ok());
	let encoding_name = content_type
		.as_ref()
		.and_then(|mime| mime.get_param("charset").map(|charset| charset.as_str()))
		.unwrap_or("utf-8");
	let encoding = Encoding::for_label(encoding_name.as_bytes()).unwrap_or(UTF_8);

	let (text, _, _) = encoding.decode(bytes);
	text.into_owned()
}
