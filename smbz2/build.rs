// SPDX-License-Identifier: GPL-3.0-or-later
// Copyright 2022 rtldg <rtldg@protonmail.com>
// This file is part of srcwrtimer (https://github.com/srcwr/srcwrtimer/)

use extshared_build_helper::*;

fn main() {
	let outdir = std::env::var("OUT_DIR").unwrap();
	generate_inc_defines_and_enums(&outdir, "../srcwrtimer/addons/sourcemod/scripting/include/bzip2.inc", "BZIP2");

	do_cbindgen();

	let build = smext_build();
	compile_lib(build, "smext");
}
