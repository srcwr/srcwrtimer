// SPDX-License-Identifier: GPL-3.0-or-later
// Copyright 2022 rtldg <rtldg@protonmail.com>
// This file is part of srcwrtimer (https://github.com/srcwr/srcwrtimer/)

#pragma once

#include <stddef.h>

class IFileObject;
namespace SourceMod {
class IChangeableForward;
}
using namespace SourceMod;

typedef unsigned bigbool;

extern "C" {

void rust_handle_destroy_SRCWRHTTPReq(void* object);
void rust_handle_destroy_SRCWRHTTPResp(void* object);
void rust_handle_destroy_SRCWRWebsocket(void* object);
void rust_handle_destroy_SRCWRWebsocketMsg(void* object);
bool rust_handle_size_SRCWRHTTPReq(void* object, unsigned int* size);
bool rust_handle_size_SRCWRHTTPResp(void* object, unsigned int* size);
bool rust_handle_size_SRCWRWebsocket(void* object, unsigned int* size);
bool rust_handle_size_SRCWRWebsocketMsg(void* object, unsigned int* size);

void* rust_SRCWRHTTPReq_SRCWRHTTPReq(const char* url);

void rust_SRCWRHTTPReq_YEET(void* object, IChangeableForward* forward, int value, const char* method);
bigbool rust_SRCWRHTTPReq_Download(void* object, IChangeableForward* forward, int value, const char* sp_path, const char* real_path);

bigbool rust_SRCWRHTTPReq_method(void* object, const char* method);
bigbool rust_SRCWRHTTPReq_header(void* object, const char* key, const char* value);
bigbool rust_SRCWRHTTPReq_query_param(void* object, const char* key, const char* value);

bigbool rust_SRCWRHTTPReq_body_set(void* object, int end, const char* content);
bigbool rust_SRCWRHTTPReq_body_add_file_on_send(void* object, const char* path);
bigbool rust_SRCWRHTTPReq_body_add(void* object, int end, const char* content);
bigbool rust_SRCWRHTTPReq_body_add_file(void* object, const char* path);
bigbool rust_SRCWRHTTPReq_body_add_file_handle(void* object, IFileObject* fileobject);
//bigbool rust_SRCWRHTTPReq_body_add_json(void* object, void* jsonobject);

bigbool rust_SRCWRHTTPReq_local_address(void* object, const char* addr);
bigbool rust_SRCWRHTTPReq_basic_auth(void* object, const char* username, const char* password);
void rust_SRCWRHTTPReq_allow_invalid_certs(void* object);

unsigned rust_SRCWRHTTPReq_max_redirections_get(void* object);
unsigned rust_SRCWRHTTPReq_timeout_get(void* object);
unsigned rust_SRCWRHTTPReq_connect_timeout_get(void* object);
void rust_SRCWRHTTPReq_max_redirections_set(void* object, unsigned redirections);
void rust_SRCWRHTTPReq_timeout_set(void* object, unsigned milliseconds);
void rust_SRCWRHTTPReq_connect_timeout_set(void* object, unsigned milliseconds);

int rust_SRCWRHTTPResp_header(void* object, const char* name, char* buffer, int maxlength);
int rust_SRCWRHTTPResp_get(void* object, char* buf, int buflen, int flags);
int rust_SRCWRHTTPResp_status_get(void* object);
unsigned rust_SRCWRHTTPResp_text_length_get(void* object);
unsigned rust_SRCWRHTTPResp_byte_length_get(void* object);
const char* rust_SRCWRHTTPResp_json_get_inner(void* object, size_t* outlen);


void* rust_SRCWRWebsocket_SRCWRWebsocket();
void rust_SRCWRWebsocket_set_handle(void* object, unsigned handle);
// For these, `object` is actually a `u32` streamid...
bigbool rust_SRCWRWebsocket_header(void* object, const char* key, const char* value);
bigbool rust_SRCWRWebsocket_write_json(void* object, void* json);
bigbool rust_SRCWRWebsocket_write_str(void* object, const char* str);
bigbool rust_SRCWRWebsocket_YEET(void* object, const char* url, unsigned user_value, IChangeableForward* fw_connection, IChangeableForward* fw_msg);

int rust_SRCWRWebsocketMsg_get(void* object, char* buf, int buflen, int flags);
unsigned rust_SRCWRWebsocketMsg_length_get(void* object);
const char* rust_SRCWRWebsocketMsg_json_get_inner(void* object, size_t* outlen);

}
