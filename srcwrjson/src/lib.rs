// SPDX-License-Identifier: GPL-3.0-or-later
// Copyright 2022-2025 rtldg <rtldg@protonmail.com>
// This file is part of srcwrtimer (https://github.com/srcwr/srcwrtimer/)

#![allow(non_snake_case)]
#![feature(let_chains)]

mod natives_json;

// Link to this because mimalloc calls `OpenProcessToken`, `AdjustTokenPrivileges`, and `LookupPrivilegeValueA` in `win_enable_large_os_pages`.
#[cfg(target_family = "windows")]
#[link(name = "advapi32")]
unsafe extern "C" {}

#[global_allocator]
static GLOBAL: mimalloc::MiMalloc = mimalloc::MiMalloc;

extshared::smext_conf_boilerplate_extension_info!();
extshared::smext_conf_boilerplate_load_funcs!();
