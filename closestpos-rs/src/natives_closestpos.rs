// SPDX-License-Identifier: GPL-3.0-or-later
// Copyright 2024 rtldg <rtldg@protonmail.com>
// This file is part of srcwrtimer (https://github.com/srcwr/srcwrtimer/)

#![allow(unused_variables)]
#![allow(non_snake_case)]

use core::ptr::NonNull;

use kiddo::immutable::float::kdtree::ImmutableKdTree;
use kiddo::float::distance::SquaredEuclidean;

pub struct ClosestPos {
	tree:     ImmutableKdTree<f32, u32, 3, 32>,
	startidx: u32,
}

#[unsafe(no_mangle)]
pub extern "C" fn rust_handle_destroy_ClosestPos(object: *mut ClosestPos) {
	let object = unsafe { Box::from_raw(object) };
	drop(object);
}
#[unsafe(no_mangle)]
pub extern "C" fn rust_handle_size_ClosestPos(object: &ClosestPos, size: &mut u32) -> bool {
	false
}

#[unsafe(no_mangle)]
pub extern "C" fn rust_ClosestPos_Create(
	points: *const [f32; 3],
	count: usize,
	startidx: u32,
) -> Option<NonNull<ClosestPos>> {
	let points = unsafe { std::slice::from_raw_parts(points, count) };
	let boxed = Box::new(ClosestPos {
		tree: ImmutableKdTree::new_from_slice(points),
		startidx,
	});
	Some(unsafe { NonNull::new_unchecked(Box::into_raw(boxed)) })
}

#[unsafe(no_mangle)]
pub extern "C" fn rust_ClosestPos_Find(object: &ClosestPos, point: &[f32; 3]) -> u32 {
	object.tree.nearest_one::<SquaredEuclidean>(point).item + object.startidx
}
