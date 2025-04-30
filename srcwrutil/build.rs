// SPDX-License-Identifier: GPL-3.0-or-later
// Copyright 2022 rtldg <rtldg@protonmail.com>
// This file is part of srcwrtimer (https://github.com/srcwr/srcwrtimer/)

use extshared_build_helper::*;

fn main() {
	do_cbindgen();
	let mut build = smext_build();
	smext_css(&mut build);
	link_sm_detours(&mut build);
	use_fileobject(&mut build);
	compile_lib(build, "smext");
}
