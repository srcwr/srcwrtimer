// SPDX-License-Identifier: GPL-3.0-or-later
// Copyright 2022-2024 rtldg <rtldg@protonmail.com>
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

pub fn generate_inc_defines_and_enums(outdir: &str, incfile: &str, name: &str) {
	println!("cargo:rerun-if-changed={}", incfile);
	let content = std::fs::read_to_string(incfile).unwrap();
	let mut defines = String::new();

	let re = regex::Regex::new(r"[^/]#define (\w+)\s+\((.+)\)").unwrap();
	for cap in re.captures_iter(&content) {
		defines.push_str(&format!(
			"#[allow(dead_code)] const {}: i32 = {};\n",
			&cap[1], &cap[2]
		));
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

pub fn use_atcprintf(build: &mut cc::Build) -> &mut cc::Build {
	println!("cargo:rerun-if-changed=../extshared/src/sprintf.cpp");
	build.file("../extshared/src/sprintf.cpp")
}

pub fn use_cellarray(build: &mut cc::Build) -> &mut cc::Build {
	println!("cargo:rerun-if-changed=../extshared/src/ICellArray.cpp");
	build
		.define("HANDLE_CELLARRAY", None)
		.file("../extshared/src/ICellArray.cpp")
}

pub fn use_fileobject(build: &mut cc::Build) -> &mut cc::Build {
	println!("cargo:rerun-if-changed=../extshared/src/IFileObject.cpp");
	println!("cargo:rerun-if-changed=../extshared/src/IFileObject.hpp");
	build
		.define("HANDLE_FILEOBJECT", None)
		.file("../extshared/src/IFileObject.cpp")
}

pub fn use_valvefs(build: &mut cc::Build) -> &mut cc::Build {
	println!("cargo:rerun-if-changed=../extshared/src/valvefs.cpp");
	build.file("../extshared/src/valvefs.cpp")
}

pub fn compile_lib(build: cc::Build, name: &str) {
	build.compile(name);
	let outdir = std::env::var("OUT_DIR").unwrap();
	let like_msvc = build.get_compiler().is_like_msvc();
	let target_windows = std::env::var("CARGO_CFG_TARGET_OS").unwrap() == "windows";

	// dumb
	if !like_msvc && target_windows {
		let _ = std::fs::copy(
			format!("{}/lib{}.a", outdir, name),
			format!("{}/{}.lib", outdir, name),
		);
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
		if name.ends_with(".cpp")
			|| name.ends_with(".hpp")
			|| name.ends_with(".h")
			|| name.ends_with(".c")
		{
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
		if name.ends_with(".cpp")
			|| name.ends_with(".hpp")
			|| name.ends_with(".h")
			|| name.ends_with(".c")
		{
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
	let sdk_path = std::env::var("HL2SDK")
		.unwrap_or(format!("../_external/alliedmodders/hl2sdk-{}", sdk_name));

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
		.include(format!("{}/common", sdk_path))
		.define(
			format!("SE_{}", sdk_name.to_ascii_uppercase()).as_str(),
			sdk_id.to_string().as_str(),
		)
		.define("SOURCE_ENGINE", sdk_id.to_string().as_str());

	if !target_windows {
		build
			.define("NO_HOOK_MALLOC", None)
			.define("NO_MALLOC_OVERRIDE", None);
	}

	if like_msvc {
		//build.define("", None);
	}

	let (lib_folder, links) = if target_windows {
		(format!("{}/lib/public", sdk_path), vec![
			"tier0", "tier1", "vstdlib", "mathlib",
		])
	} else {
		(
			format!("{}/lib/linux", sdk_path),
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
	let sm =
		std::env::var("SOURCEMOD").unwrap_or("../_external/alliedmodders/sourcemod".to_string());
	let target_windows = std::env::var("CARGO_CFG_TARGET_OS").unwrap() == "windows";

	slurp_folder(mainbuild, &format!("{}/public/CDetour", sm));

	let mut detours_build = cc::Build::new();
	detours_build
		.include(format!("{}/public", sm))
		.flag_if_supported("-Wno-sign-compare");
	if target_windows {
		detours_build
			.define("_CRT_SECURE_NO_WARNINGS", None)
			.define("WIN32", None)
			.define("_WINDOWS", None);
	} else {
		detours_build
			.define("HAVE_STRING_H", "1")
			.define("_LINUX", None)
			.define("POSIX", None);
	}

	slurp_folder(&mut detours_build, &format!("{}/public/asm", sm));
	slurp_folder(&mut detours_build, &format!("{}/public/libudis86", sm));
	detours_build.compile("detours_and_shit");
}

pub fn smext_build() -> cc::Build {
	let sm =
		std::env::var("SOURCEMOD").unwrap_or("../_external/alliedmodders/sourcemod".to_string());
	// TODO: mmsource-1.12
	let mm =
		std::env::var("METAMOD").unwrap_or("../_external/alliedmodders/mmsource-1.11".to_string());

	println!("cargo:rerun-if-changed=../extshared/src/coreident.cpp");
	println!("cargo:rerun-if-changed=../extshared/src/coreident.hpp");
	println!("cargo:rerun-if-changed=../extshared/src/extension.cpp");
	println!("cargo:rerun-if-changed=../extshared/src/extension.h");
	println!("cargo:rerun-if-changed=../extshared/src/rust_exports.h");
	println!("cargo:rerun-if-changed=../extshared/src/smsdk_config.h");
	//rerun_on_dir_cc_files_changed("../extshared/src");

	let mut build = cc::Build::new();
	let like_msvc = build.get_compiler().is_like_msvc();
	let target_windows = std::env::var("CARGO_CFG_TARGET_OS").unwrap() == "windows";

	build
		.define("SM_LOGIC", None) // needed for getting coreident.hpp & alliedmodders/sourcemod/core/logic/HandleSys.h .... although i don't use it...
		.include(format!("{}/core", sm))
		.include(format!("{}/public", sm))
		.include(format!("{}/public/extensions", sm))
		.include(format!("{}/sourcepawn/include", sm))
		.include(format!("{}/public/amtl/amtl", sm))
		.include(format!("{}/public/amtl", sm))
		.include(format!("{}/core", mm))
		.include(format!("{}/core/sourcehook", mm))
		.include("../extshared/src")
		.file("../extshared/src/coreident.cpp")
		.file("../extshared/src/extension.cpp")
		.file(format!("{}/public/smsdk_ext.cpp", sm))
		.cpp(true)
		.static_crt(true);

	if like_msvc {
		// We can change the .pdb name if we want here...
		// println!("cargo::rustc-link-arg=/PDB:_build\\i686-pc-windows-msvc\\release\\{}.ext.pdb", std::env::var("CARGO_PKG_NAME").unwrap());
		// Same with .dll file name...
		// println!("cargo::rustc-link-arg=/OUT:_build\\i686-pc-windows-msvc\\release\\{}.ext.dll", std::env::var("CARGO_PKG_NAME").unwrap());

		build
			.flag("/std:c++latest") // /std:c++23 doesn't exist yet!! crazy! TODO: periodically check this https://learn.microsoft.com/en-us/cpp/build/reference/std-specify-language-standard-version
			.flag("/wd4100") // disable warning C4100: unreferenced formal parameter
			.flag("/EHsc")
			// "This needs to be after our optimization flags which could otherwise disable it."
			// "Don't omit the frame pointer.""
			.flag("/Oy-");
	} else {
		build
			.define("stricmp", "strcasecmp")
			.define("_stricmp", "strcasecmp")
			.define("HAVE_STDINT_H", None)
			.define("GNUC", None)
			.flag("-m32")
			.flag("-msse")
			.flag("-std=c++23")
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
		build
			.define("_CRT_SECURE_NO_WARNINGS", None)
			.define("WIN32", None)
			.define("_WINDOWS", None);
	} else {
		build
			.define("_vsnprintf", "vsnprintf")
			.define("_snprintf", "snprintf")
			.define("_LINUX", None)
			.define("GetSMExtAPI", "GetSMExtAPIxxx")
			.define("POSIX", None);
	}

	// slurps the extension's src dir (not the src dir build_template.rs is in)
	slurp_folder(&mut build, "src");

	build
}
