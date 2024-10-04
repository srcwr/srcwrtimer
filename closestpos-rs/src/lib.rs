// SPDX-License-Identifier: GPL-3.0-or-later
// Copyright 2024 rtldg <rtldg@protonmail.com>
// This file is part of srcwrtimer (https://github.com/srcwr/srcwrtimer/)

//#![allow(unused_variables)]
//#![allow(non_snake_case)]

mod natives_closestpos;

// TODO: IDK why but kiddo unwraps a None Option when I switch to bhop_badges.
// https://github.com/sdd/kiddo/blob/460442f4fde309290bdb59e2d4e43dccaa1f3a6d/src/immutable/float/construction.rs#L92
// Maybe I fucked up pointer/array FFI somewhere... don't care right now though...

extshared::smext_conf_boilerplate_extension_info!();
extshared::smext_conf_boilerplate_load_funcs!();
