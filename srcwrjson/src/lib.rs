// SPDX-License-Identifier: GPL-3.0-or-later
// Copyright 2022 rtldg <rtldg@protonmail.com>
// This file is part of srcwrtimer (https://github.com/srcwr/srcwrtimer/)

#![allow(non_snake_case)]
#![feature(let_chains)]

mod natives_json;

#[global_allocator]
static GLOBAL: mimalloc::MiMalloc = mimalloc::MiMalloc;

extshared::smext_conf_boilerplate_extension_info!();
extshared::smext_conf_boilerplate_load_funcs!();
