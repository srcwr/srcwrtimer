
[![Discord server](https://discordapp.com/api/guilds/389675819959844865/widget.png?style=shield)](https://discord.gg/JB8rxyD6uZ) (bhoptimer discord)

## Frequently asked question (FAQ)

### What is this?
`srcwr`'s goal is to be the [SourceJump](https://sourcejump.net/) "killer". (It's pretty far from this goal)

`srcwrtimer`'s goal is to be a successor to [bhoptimer](https://github.com/shavitush/bhoptimer) to make `srcwr` possible. (Also pretty far from this goal)

`srcwrtimer` currently contains SourceMod extensions that use C++/Rust and are easy to build.

So you might be interested in this repository right now if:
- You want to use Rust in a SourceMod extension.
- You want to build C++ SourceMod extensions easier*.
- You want to cross-compile extensions to Linux from Windows.

### Is this a fork of bhoptimer?
No, but A LOT of code and inspiration/ideas come from it.

### Can I have an overview of the components in this repository?
The [AMBuild](https://wiki.alliedmods.net/AMBuild) build-system used by SourceMod C++ extensions is ass, compiling extensions for Linux is ass, cross-compiling between operating-systems is ass, and managing dependencies for C++ is ass.

For these reasons: `srcwrtimer` extensions are C++ and Rust hybrids.
AMBuild is avoided. Compiling the extensions for Linux is easy, even from Windows. And dependencies (assuming they're Rust crates haha) are easy to use.

[cargo-make](https://sagiegurari.github.io/cargo-make/) is used as part of the AMBuild replacement and the other half of it goes to Rust's built-in [build.rs](https://doc.rust-lang.org/cargo/reference/build-scripts.html) system.
[Zig](https://ziglang.org/)'s embedded C/C++ cross-compiler is used to compile extensions for Linux and also target Source-engine compatible `glibc` versions (via [cargo-zigbuild](https://github.com/rust-cross/cargo-zigbuild).
TODO: investigate https://github.com/rust-cross/cargo-xwin for msvc builds from linux

### Extensions:
- closestpos
  - [sm_closestpos](https://github.com/rtldg/sm_closestpos) is really annoying to build (because it's a SourceMod extension) so it was merged into *this* repo to be easily buildable.
- extshared
  - The base extension code that is shared by the other extensions. It does a lot to make Rust usable.
- extshared_build_helper
  - Extension build helper code. Basically the thing that compiles C++ code.
- smbz2
  - A good sample extension. [SMbz2](https://github.com/Versatile-BFG/SMbz2) is really annoying to build (because it's a SouceMod extension) so this was made to replicate its functionality.
- srcwrf64
  - Unused/incomplete/experimental float64 extension. Probably would be similar to [double_ext](https://github.com/XutaxKamay/double_ext) but without handles.
- srcwrhttp
  - A better HTTP(S) extension for plugins to use. Provides WebSocket support too. Minimally tested.
- srcwrhttpsrv
  - Was going to be a rewrite of [conplex + webcon](https://forums.alliedmods.net/showthread.php?t=270962) but this never happened.
- srcwrjson
  - A better JSON extension for plugins to use. The `natives_json.rs` code is a mess lol. Minimally tested.
- srcwrreplayman
  - An incomplete/placeholder extension to provide more efficient and performant replay storage and access. Expect something similar to [bhoptimer_helper_minimal](https://github.com/srcwr/bhoptimer_helper_minimal).
- srcwrsample
  - A sample/test extension that I use for random things. If you want an extension to copy & paste to base a new extension off of then you could try this, srcwrutil, or smbz2.
- srcwrsql
  - Was going to be a SQL extension that runs queries parallel to each other. SourceMod's SQL extension runs each query one-after-another, with a sleep after each query, so it's not very performant. TODO one day...
- srcwrsyncer
  - Experimental git repo clone/pull -er.
- srcwrutil
  - A bunch of miscellaneous things like SHA1 hashing and download-table hooking. Blocks `.nav` files from being created server-side, and downloaded client-side.


### Plugins in srcwrtimer:


### Why doesn't this use C++ helper crates?
I just didn't... I could say it's simpler or easier to keep the C++ separate, which might be true. There's obvious value in using bindgen/cbindgen so you don't mess up FFI function declarations. The inline C++ with the `cpp` crate looks very useful. The original srcwrtimer extensions mainly keep the SourceMod interfacing on the C++ side which calls to Rust functions to natives, so there's not a lot of fancy `Rust <-> C++` FFI going on.

Helper crates (which aren't used):
- [cpp](https://crates.io/crates/cpp)
  - Allows you to embed C++ code in a Rust file.
- [cxx](https://crates.io/crates/cxx)
  - A `Rust <-> C++` binding/bridging crate.
- [autocxx](https://crates.io/crates/autocxx)
  - A more magical cxx.
- [bindgen](https://crates.io/crates/bindgen)
  - Generate Rust bindings to C/C++ libraries...
- [cbindgen](https://crates.io/crates/cbindgen)
  - Generate C bindings to Rust libraries...
    As of right now (2024-09-30) this seems to have trouble with the `#[unsafe(no_mangle)]` that nightly Rust wants me to use...

### Why does this use C++ at all?
Interfacing with SourceMod through Rust can be tedious. You *can* do it in pure Rust thanks to the [sm-ext-rs](https://github.com/srcwr/sm-ext-rs) (srcwr fork) crate that [asherkin](https://github.com/asherkin) made, but lacking hl2sdk support makes it impractical for many things.

### What is developing this like and what's used?
I use VSCode with the `rust-analyzer` extension (& nightly Rust) and the entire cargo workspace open at once. It's not that fast with "check on save" enabled, and it would probably work if you were to open a crate by itself, but jumping around the projects is slightly less convenient then. The "intellisense" annotations aren't great when viewing C++ files because the `build.rs` handles C++ header include paths at build and isn't accessible to the C++ VSCode extensions, and you get some dumb things like `namespace "std" has no member "unordered_map"` for some reason.

### What is the `_build` folder?
Rust builds binaries in a folder named `target` by default but I want the build folder to be a bit higher up in my file-explorer so I changed it...

### What is the `_external` folder?
That's where dependencies are downloaded to, such as Zig, SourceMod, Metamod: Source, and hl2sdk. It's easier this way for now but the paths for those dependencies will likely be configurable in the future.

### What is the `_package` folder?
That's where all the SourceMod files are dumped after being built. Extensions, plugins, gamedata, translations, etc.

## TODO:
- We don't have a good system for building an extension for different hl2sdk versions simultaneously without just copy-pasting.
  - Probably requires `Makefile.toml` changes & new tasks for TF2 / CS:S specific builds and stuff and yada yada yada. (the build.rs would then check the features to determine which sdk to use...)
- Investigate the C++ helper crates more...
- Cleanup srcwrhttp & srcwrjson... Document natives & provide more examples in `json.inc` & `http.inc`...
- Do something about including `target = "i686-pc-windows-msvc"` in `.cargo/config.toml` since that might be a bit annoying for Linux developers.
- Make it possible to use different paths for the things in `_external`.
- Don't forget about [`Allow natives to return structs and arrays.`](https://github.com/alliedmodders/sourcepawn/pull/988)

## build stuff
```
# install git and rustup (& MSVC build tools if you're on Windows)
# then install cargo-make...
cargo install --force cargo-make
# this will download zig, metamod-source, sourcemod, and hl2sdk... (and rust toolchains for i686-pc-windows-msvc & i686-unknown-linux-gnu), then it'll build things
cargo make full
```

## Contributing
Contributions are welcome as long as they follow the same license/terms as the original file, or in the case of a new file, as long as the license/terms are compatible with GPL version 3.0.

## License
Most code is licensed as [GPL-3.0-or-later](https://www.gnu.org/licenses/gpl-3.0.en.html).

There are many exceptions to this though and you can find more info in each individual file.

You can also use [`reuse`](https://github.com/fsfe/reuse-tool) to check licenses or to generate spdx info for the project.
```sh
pip3 install --user reuse
reuse lint
reuse spdx
```
Also, `cargo-license` is another useful tool which gives insight to the included Rust dependencies.
And `cargo bundle-licenses --format json --output THIRDPARTY.json`

