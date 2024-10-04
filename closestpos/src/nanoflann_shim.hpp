// SPDX-License-Identifier: BSD-2-Clause
// Copyright 2011-2021 Jose Luis Blanco (joseluisblancoc@gmail.com).
// Copyright 2021-2024 rtldg <rtldg@protonamil.com>

/***********************************************************************
 * Software License Agreement (BSD License)
 *
 * Copyright 2011-2021 Jose Luis Blanco (joseluisblancoc@gmail.com).
 * Copyright 2021-2024 rtldg <rtldg@protonamil.com>
 *   All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 *
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 *
 * THIS SOFTWARE IS PROVIDED BY THE AUTHOR ``AS IS'' AND ANY EXPRESS OR
 * IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES
 * OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED.
 * IN NO EVENT SHALL THE AUTHOR BE LIABLE FOR ANY DIRECT, INDIRECT,
 * INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT
 * NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
 * DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY
 * THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF
 * THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 *************************************************************************/

#pragma once

#include "nanoflann.hpp"
using namespace nanoflann;

struct Point
{
	float pos[3];
	int idx;
};

struct PointCloud
{
	const Point* pts;
	size_t pts_size;

	inline size_t kdtree_get_point_count() const { return pts_size; }

	inline float kdtree_get_pt(const size_t idx, const size_t dim) const
	{
		if (dim == 0) return pts[idx].pos[0];
		else if (dim == 1) return pts[idx].pos[1];
		else return pts[idx].pos[2];
		//return pts[idx].pos[dim];
	}

	template <class BBOX>
	bool kdtree_get_bbox(BBOX& /* bb */) const { return false; }
};

typedef KDTreeSingleIndexAdaptor<
	// L2_Simple_Adaptor<float, PointCloud>,
	L2_Adaptor<float, PointCloud>,
	PointCloud,
	3 /*dimensions*/
	> my_kd_tree_t;

class KDTreeContainer
{
public:
	PointCloud cloud;
	my_kd_tree_t *index;

	~KDTreeContainer()
	{
		delete index;
	}
};

extern "C" {
KDTreeContainer* nanoflann_shim_create_container(const Point* pts, size_t pts_size)
{
	KDTreeContainer *container = new KDTreeContainer();
	container->cloud.pts = pts;
	container->cloud.pts_size = pts_size;
	// TODO: Would be good to try catching exceptions here if necessary...
	container->index = new my_kd_tree_t(3, container->cloud, {30 /* max leaf */});
	return container;
}
void nanoflann_shim_delete_container(KDTreeContainer *container)
{
	delete container;
}
unsigned nanoflann_shim_get_nearest(KDTreeContainer *container, float *query_pt)
{
	float out_dist_sqr;
	size_t num_results = 1;
	uint32_t ret_index = 0;
	container->index->knnSearch(query_pt, num_results, &ret_index, &out_dist_sqr);
	return ret_index;
	// return container->cloud.pts[ret_index].idx;
}
size_t nanoflann_shim_get_used_memory(KDTreeContainer *container)
{
	return container->index->usedMemory(*container->index);
}
} // extern "C"
