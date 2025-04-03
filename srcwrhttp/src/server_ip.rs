// SPDX-License-Identifier: WTFPL
// Copyright 2022 rtldg <rtldg@protonmail.com>
// This file is part of srcwrtimer (https://github.com/srcwr/srcwrtimer/)

// Some server hosts (NFOservers) send HTTP requests from the default IP.
// Since they have a lot of servers & IPs per box, that default IP is NOT
// the current srcds instance's public IP.
// So this shit is so we can bind the address used for the HTTP client.

// Old note to self:
//  Windows: bind() then connect(). Socket2 lib for Rust?
//  Linux: SO_BINDTODEVICE?

use std::net::IpAddr;
use std::net::Ipv4Addr;
use std::str::FromStr;

use crate::ip_addr;

unsafe extern "C" {
	pub fn SteamWorks_GetPublicIP() -> u32;
}

#[allow(dead_code)]
pub fn steamworks_publicip() -> Option<IpAddr> {
	let publicip = unsafe { SteamWorks_GetPublicIP() };
	if publicip == 0 {
		None
	} else {
		Some(Ipv4Addr::from(publicip).into())
	}
}

pub fn commandline_bindip() -> Option<IpAddr> {
	let mut next_is_ip = false;
	for a in std::env::args() {
		if next_is_ip {
			let ip = Ipv4Addr::from_str(&a).ok()?;
			return if ip_addr::is_global(&ip) { Some(ip.into()) } else { None };
		}
		next_is_ip = a == "-ip" || a == "+ip";
	}
	None
}
