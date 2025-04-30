// SPDX-License-Identifier: GPL-3.0-or-later
// Copyright 2022-2025 rtldg <rtldg@protonmail.com>
// This file is part of srcwrtimer (https://github.com/srcwr/srcwrtimer/)

/*
fn qenv(s: &str) -> String {
  format!("\"{}\"", std::env::var(s).unwrap())
}
*/

/*
#[allow(dead_code)]
fn file2<P: AsRef<std::path::Path>>(build: &mut cc::Build, p: P) -> &mut cc::Build {
  println!("cargo:rerun-if-changed={}", p.as_ref().display());
  build.file(p)
}
*/

use vergen_gitcl::BuildBuilder;
use vergen_gitcl::Emitter;
use vergen_gitcl::GitclBuilder;

static SRCWRTIMER_ROOT_DIR: std::sync::LazyLock<String> =
	std::sync::LazyLock::new(|| std::env::var("SRCWRTIMER_ROOT_DIR").unwrap());
static IS_X64: std::sync::LazyLock<bool> =
	std::sync::LazyLock::new(|| std::env::var("CARGO_CFG_TARGET_POINTER_WIDTH").unwrap() == "64");

pub fn generate_inc_defines_and_enums(outdir: &str, incfile: &str, name: &str) {
	println!("cargo:rerun-if-changed={}", incfile);
	let content = std::fs::read_to_string(incfile).unwrap();
	let mut defines = String::new();

	let re = regex::Regex::new(r"[^/]#define (\w+)\s+\((.+)\)").unwrap();
	for cap in re.captures_iter(&content) {
		defines.push_str(&format!("#[allow(dead_code)] const {}: i32 = {};\n", &cap[1], &cap[2]));
	}

	let re = regex::RegexBuilder::new(r"(enum \w+\s*\{.*?\})")
		.dot_matches_new_line(true)
		.build()
		.unwrap();
	for cap in re.captures_iter(&content) {
		defines.push_str(&format!(
			"
#[allow(dead_code)]
#[allow(non_camel_case_types)]
#[allow(clippy::enum_variant_names)]
#[derive(Copy, Debug, Clone)]
#[repr(C)]
pub {}\n",
			&cap[1]
		));
	}

	std::fs::write(format!("{}/{}_DEFINES.rs", outdir, name), defines).unwrap();
}

pub fn use_atcprintf(build: &mut cc::Build) {
	slurp_single_file(build, &format!("{}/extshared/src/sprintf.cpp", *SRCWRTIMER_ROOT_DIR));
}

pub fn use_cellarray(build: &mut cc::Build) {
	build.define("HANDLE_CELLARRAY", None);
	slurp_single_file(build, &format!("{}/extshared/src/ICellArray.cpp", *SRCWRTIMER_ROOT_DIR));
}

pub fn use_fileobject(build: &mut cc::Build) {
	build.define("HANDLE_FILEOBJECT", None);
	slurp_single_file(build, &format!("{}/extshared/src/IFileObject.cpp", *SRCWRTIMER_ROOT_DIR));
	slurp_single_file(build, &format!("{}/extshared/src/IFileObject.hpp", *SRCWRTIMER_ROOT_DIR));
}

pub fn use_valvefs(build: &mut cc::Build) {
	slurp_single_file(build, &format!("{}/extshared/src/valvefs.cpp", *SRCWRTIMER_ROOT_DIR));
}

pub fn compile_lib(build: cc::Build, name: &str) {
	build.compile(name);
	let outdir = std::env::var("OUT_DIR").unwrap();
	let like_msvc = build.get_compiler().is_like_msvc();
	let target_windows = std::env::var("CARGO_CFG_TARGET_OS").unwrap() == "windows";

	// dumb
	if !like_msvc && target_windows {
		let _ = std::fs::copy(format!("{}/lib{}.a", outdir, name), format!("{}/{}.lib", outdir, name));
	}
}

pub fn slurp_single_file(build: &mut cc::Build, name: &str) {
	println!("cargo:rerun-if-changed={}", name);
	if name.ends_with(".cpp") || name.ends_with(".c") {
		build.file(name);
	}
}

pub fn slurp_folder(build: &mut cc::Build, dirrrr: &str) {
	for path in std::fs::read_dir(dirrrr).unwrap() {
		let name = path.unwrap().file_name().into_string().unwrap();
		if name.ends_with(".cpp") || name.ends_with(".hpp") || name.ends_with(".h") || name.ends_with(".c") {
			let full = format!("{}/{}", dirrrr, name);
			println!("cargo:rerun-if-changed={}", full);
			if name.ends_with(".cpp") || name.ends_with(".c") {
				build.file(full);
			}
		}
	}
}

pub fn rerun_on_dir_cc_files_changed(dirrrr: &str) {
	for path in std::fs::read_dir(dirrrr).unwrap() {
		let name = path.unwrap().file_name().into_string().unwrap();
		if name.ends_with(".cpp") || name.ends_with(".hpp") || name.ends_with(".h") || name.ends_with(".c") {
			println!("cargo:rerun-if-changed={}/{}", dirrrr, name);
		}
	}
}

pub fn smext_css(build: &mut cc::Build) {
	smext_hl2sdk_for_good_games(build, "css", 6);
}

pub fn smext_tf2(build: &mut cc::Build) {
	smext_hl2sdk_for_good_games(build, "tf2", 12);
}

// make an smext_hl2sdk_for_bad_games if you're not using the CSS or TF2 hl2sdk...
// https://github.com/alliedmodders/sourcemod/blob/master/public/sample_ext/AMBuildScript
pub fn smext_hl2sdk_for_good_games(build: &mut cc::Build, sdk_name: &str, sdk_id: usize) {
	let sdk_path = std::env::var("HL2SDK").unwrap_or(format!(
		"{}/_external/alliedmodders/hl2sdk-{}",
		*SRCWRTIMER_ROOT_DIR, sdk_name
	));

	let like_msvc = build.get_compiler().is_like_msvc();
	let target_windows = std::env::var("CARGO_CFG_TARGET_OS").unwrap() == "windows";

	build
		.include(format!("{}/public", sdk_path))
		.include(format!("{}/public/engine", sdk_path))
		.include(format!("{}/public/mathlib", sdk_path))
		.include(format!("{}/public/vstdlib", sdk_path))
		.include(format!("{}/public/tier0", sdk_path))
		.include(format!("{}/public/tier1", sdk_path))
		.include(format!("{}/public/game/server", sdk_path))
		.include(format!("{}/public/toolframework", sdk_path))
		.include(format!("{}/game/shared", sdk_path))
		.include(format!("{}/game/server", sdk_path))
		.include(format!("{}/common", sdk_path))
		.define("GAME_DLL", None)
		.define(
			format!("SE_{}", sdk_name.to_ascii_uppercase()).as_str(),
			sdk_id.to_string().as_str(),
		)
		.define("SOURCE_ENGINE", sdk_id.to_string().as_str());

	if !target_windows {
		build
			.define("NO_HOOK_MALLOC", None)
			.define("NO_MALLOC_OVERRIDE", None)
			.define("LINUX", None);
	}

	if like_msvc {
		if *IS_X64 {
			build.define("COMPILER_MSVC64", "1");
		} else {
			build.define("COMPILER_MSVC32", "1");
		}
		build.define("COMPILER_MSVC", "1");
	} else {
		build.define("COMPILER_GCC", "1");
	}

	let (lib_folder, links) = if target_windows {
		(
			if *IS_X64 {
				format!("{sdk_path}/lib/public/x64")
			} else {
				format!("{sdk_path}/lib/public/x86")
			},
			vec!["tier0", "tier1", "vstdlib", "mathlib"],
		)
	} else {
		(
			if *IS_X64 {
				format!("{sdk_path}/lib/public/linux64")
			} else {
				format!("{sdk_path}/lib/public/linux")
			},
			// vec!["tier1_i486", "mathlib_i486"] // ??????
			vec![],
		)
	};

	let lib_folder = std::fs::canonicalize(lib_folder).unwrap();
	println!("cargo:rustc-link-search={}", lib_folder.display());
	for link in links {
		println!("cargo:rustc-link-lib={}", link);
	}
}

// This is necessary because the CC compiler (zig cc) throws errors about using C++ standards via .std() when compiling C files.... frick....
pub fn link_sm_detours(mainbuild: &mut cc::Build) {
	let sm = std::env::var("SOURCEMOD").unwrap_or(format!("{}/_external/alliedmodders/sourcemod", *SRCWRTIMER_ROOT_DIR));
	//let target_windows = std::env::var("CARGO_CFG_TARGET_OS").unwrap() == "windows";
	let like_msvc = mainbuild.get_compiler().is_like_msvc();

	mainbuild
		.include(format!("{}/public/CDetour", sm))
		.include(format!("{}/public/safetyhook/include", sm));
	// we also need to link CDetour/detours.cpp
	slurp_folder(mainbuild, &format!("{}/public/CDetour", sm));

	// zydis needs to build as a C lib because fuck idk why .file() doesn't like to work... ugh
	// something with .flag("stdverisonhere") maybe...
	let mut zydis = cc::Build::new();
	slurp_folder(&mut zydis, &format!("{}/public/safetyhook/zydis", sm));
	zydis
		.include(format!("{}/public/safetyhook/zydis", sm))
		.flag_if_supported("-Wno-unused");
	zydis.compile("zydis_and_shit");

	let mut detours = cc::Build::new();
	detours
		.cpp(true)
		.include(format!("{}/public/safetyhook/include", sm))
		.include(format!("{}/public/safetyhook/zydis", sm))
		.flag_if_supported("-Wno-sign-compare");
	slurp_folder(&mut detours, &format!("{}/public/safetyhook/src", sm));

	if like_msvc {
		if *IS_X64 {
			detours.define("COMPILER_MSVC64", "1");
		} else {
			detours.define("COMPILER_MSVC32", "1");
		}
		detours
			.define("COMPILER_MSVC", "1")
			.flag("/std:c++latest") // /std:c++23 doesn't exist yet!! crazy! TODO: periodically check this https://learn.microsoft.com/en-us/cpp/build/reference/std-specify-language-standard-version
			.flag("/permissive-")
			// C++ stack unwinding & extern "C" don't throw...
			.flag("/EHsc");
	} else {
		detours
			.define("COMPILER_GCC", "1")
			.flag("-std=c++23")
			.define("LINUX", None)
			.flag("-Wno-unknown-pragmas")
			.flag("-Wno-dangling-else")
			.flag("-Wno-deprecated-volatile");
	}

	detours.compile("safetyhook_and_shit");
}

pub fn smext_build() -> cc::Build {
	let buildinfo = BuildBuilder::default().use_local(false).build_date(true).build().unwrap();
	let gitinfo = GitclBuilder::default()
		.branch(true)
		.dirty(true)
		.sha(true)
		//.commit_date(true)
		.build()
		.unwrap();
	Emitter::default()
		.add_instructions(&buildinfo)
		.unwrap()
		.add_instructions(&gitinfo)
		.unwrap()
		.fail_on_error()
		.emit()
		.unwrap();

	let sm = std::env::var("SOURCEMOD").unwrap_or(format!("{}/_external/alliedmodders/sourcemod", *SRCWRTIMER_ROOT_DIR));
	let mm = std::env::var("METAMOD").unwrap_or(format!("{}/_external/alliedmodders/mmsource", *SRCWRTIMER_ROOT_DIR));

	println!("cargo:rerun-if-changed={}/extshared/src/coreident.cpp", *SRCWRTIMER_ROOT_DIR);
	println!("cargo:rerun-if-changed={}/extshared/src/coreident.hpp", *SRCWRTIMER_ROOT_DIR);
	println!("cargo:rerun-if-changed={}/extshared/src/extension.cpp", *SRCWRTIMER_ROOT_DIR);
	println!("cargo:rerun-if-changed={}/extshared/src/extension.h", *SRCWRTIMER_ROOT_DIR);
	println!("cargo:rerun-if-changed={}/extshared/src/rust_exports.h", *SRCWRTIMER_ROOT_DIR);
	println!("cargo:rerun-if-changed={}/extshared/src/smsdk_config.h", *SRCWRTIMER_ROOT_DIR);
	//rerun_on_dir_cc_files_changed("{}/extshared/src", *SRCWRTIMER_ROOT_DIR);

	let mut build = cc::Build::new();
	let like_msvc = build.get_compiler().is_like_msvc();
	let target_windows = std::env::var("CARGO_CFG_TARGET_OS").unwrap() == "windows";

	build
		.define("SM_LOGIC", None) // needed for getting coreident.hpp & alliedmodders/sourcemod/core/logic/HandleSys.h .... although i don't use it...
		.define("GAME_DLL", None)
		.include(format!("{}/core", sm))
		.include(format!("{}/public", sm))
		.include(format!("{}/public/extensions", sm))
		.include(format!("{}/sourcepawn/include", sm))
		.include(format!("{}/public/amtl/amtl", sm))
		.include(format!("{}/public/amtl", sm))
		.include(format!("{}/core", mm))
		.include(format!("{}/core/sourcehook", mm))
		.include(format!("{}/extshared/src", *SRCWRTIMER_ROOT_DIR))
		.file(format!("{}/extshared/src/coreident.cpp", *SRCWRTIMER_ROOT_DIR))
		.file(format!("{}/extshared/src/extension.cpp", *SRCWRTIMER_ROOT_DIR))
		.file(format!("{}/public/smsdk_ext.cpp", sm))
		.cpp(true)
		.static_crt(true);

	if like_msvc {
		// We can change the .pdb name if we want here...
		// println!("cargo::rustc-link-arg=/PDB:_build\\i686-pc-windows-msvc\\release\\{}.ext.pdb", std::env::var("CARGO_PKG_NAME").unwrap());
		// Same with .dll file name...
		// println!("cargo::rustc-link-arg=/OUT:_build\\i686-pc-windows-msvc\\release\\{}.ext.dll", std::env::var("CARGO_PKG_NAME").unwrap());

		if *IS_X64 {
			build
				.flag("/d2archSSE42") // sse4.2!
				.define("COMPILER_MSVC64", "1");
		} else {
			build.define("COMPILER_MSVC32", "1");
		}

		build
			.define("COMPILER_MSVC", "1")
			.flag("/std:c++latest") // /std:c++23 doesn't exist yet!! crazy! TODO: periodically check this https://learn.microsoft.com/en-us/cpp/build/reference/std-specify-language-standard-version
			// We also set /Zi and /FS in .cargo/config.toml with some cc-crate target-specific environment variables
			.flag("/Zi") // debug info things https://learn.microsoft.com/en-us/cpp/build/reference/z7-zi-zi-debug-information-format
			.flag("/FS") // force synchronous pdb writes https://learn.microsoft.com/en-us/cpp/build/reference/fs-force-synchronous-pdb-writes
			.flag("/wd4100") // disable warning C4100: unreferenced formal parameter
			.flag("/EHsc") // https://learn.microsoft.com/en-us/cpp/build/reference/eh-exception-handling-model
			// "This needs to be after our optimization flags which could otherwise disable it."
			// "Don't omit the frame pointer." (this doesn't do anything in x64)
			.flag("/Oy-");
	} else {
		if *IS_X64 {
			build.flag("-mcpu=x86_64_v2");
		} else {
			build.flag("-msse").flag("-m32");
		}
		build
			.define("HAVE_STDINT_H", None)
			.define("GNUC", None)
			.define("COMPILER_GCC", "1")
			.flag("-std=c++23")
			.flag("-Wno-unknown-pragmas")
			.flag("-Wno-dangling-else")
			.flag("-Wno-deprecated-volatile")
			.flag("-Wno-non-virtual-dtor")
			.flag("-Wno-overloaded-virtual")
			.flag("-Wno-implicit-exception-spec-mismatch")
			.flag("-fno-threadsafe-statics")
			//.flag("-fno-exceptions")
			.flag("-fvisibility-inlines-hidden")
			.flag("-Wno-deprecated-copy-with-user-provided-copy")
			.flag("-Wno-implicit-const-int-float-conversion")
			.flag("-Wno-expansion-to-defined")
			.flag("-Wno-inconsistent-missing-override")
			.flag("-Wno-narrowing")
			.flag("-Wno-delete-non-virtual-dtor")
			.flag("-Wno-unused-result")
			.flag("-Wno-implicit-exception-spec-mismatch")
			.flag("-Wno-deprecated-register")
			.flag("-Wno-sometimes-uninitialized")
			.flag_if_supported("-Wno-unused")
			.flag_if_supported("-Wno-unused-parameter");
	}

	if target_windows {
		if *IS_X64 {
			build.define("WIN64", None);
		}
		build
			.define("_CRT_SECURE_NO_WARNINGS", None)
			.define("WIN32", None)
			.define("_WINDOWS", None);
	} else {
		build
			.define("_LINUX", None)
			.define("LINUX", None)
			.define("GetSMExtAPI", "GetSMExtAPIxxx")
			.define("POSIX", None);
	}

	// slurps the extension's src dir (not the src dir build_template.rs is in)
	slurp_folder(&mut build, "src");

	build
}

pub fn do_cbindgen() {
	println!("cargo:rerun-if-changed=../extshared_build_helper/cbindgen.toml");
	cbindgen::generate_with_config(
		".",
		cbindgen::Config::from_file("../extshared_build_helper/cbindgen.toml")
			.expect("couldn't find ../extshared_build_helper/cbindgen.toml"),
	)
	.expect("cbindgen failed to generate headers")
	.write_to_file(format!(
		"src/rust_exports_{}.h",
		std::env::var("CARGO_PKG_NAME").unwrap()
	));
}
