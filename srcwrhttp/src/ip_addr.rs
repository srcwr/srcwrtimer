// SPDX-License-Identifier: MIT OR Apache-2.0
// Copyright (c) The Rust Project Contributors/Developers

// Snipped out of https://github.com/rust-lang/rust/blob/master/library/core/src/net/ip_addr.rs

// TODO: Wait for the `ip` feature to stablize: https://github.com/rust-lang/rust/issues/27709

use std::net::Ipv4Addr;

pub const fn is_shared(addr: &Ipv4Addr) -> bool {
	addr.octets()[0] == 100 && (addr.octets()[1] & 0b1100_0000 == 0b0100_0000)
}

pub const fn is_benchmarking(addr: &Ipv4Addr) -> bool {
	addr.octets()[0] == 198 && (addr.octets()[1] & 0xfe) == 18
}

pub const fn is_reserved(addr: &Ipv4Addr) -> bool {
	addr.octets()[0] & 240 == 240 && !addr.is_broadcast()
}

pub const fn is_global(addr: &Ipv4Addr) -> bool {
	!(addr.octets()[0] == 0 // "This network"
		|| addr.is_private()
		|| is_shared(addr)
		|| addr.is_loopback()
		|| addr.is_link_local()
		// addresses reserved for future protocols (`192.0.0.0/24`)
		// .9 and .10 are documented as globally reachable so they're excluded
		|| (
			addr.octets()[0] == 192 && addr.octets()[1] == 0 && addr.octets()[2] == 0
			&& addr.octets()[3] != 9 && addr.octets()[3] != 10
		)
		|| addr.is_documentation()
		|| is_benchmarking(addr)
		|| is_reserved(addr)
		|| addr.is_broadcast())
}
