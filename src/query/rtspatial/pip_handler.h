#pragma once
#include <optix.h>
#include <thrust/pair.h>

#include <cstdint>

#include "pip_context.h"
#include "rtspatial/utils/queue.h"
/*
 * Copyright (c) 1970-2003, Wm. Randolph Franklin

Permission is hereby granted, free of charge, to any person obtaining a copy of
this software and associated documentation files (the "Software"), to deal in
the Software without restriction, including without limitation the rights to
use, copy, modify, merge, publish, distribute, sublicense, and/or sell copies of
the Software, and to permit persons to whom the Software is furnished to do so,
subject to the following conditions:

Redistributions of source code must retain the above copyright notice, this list
of conditions and the following disclaimers. Redistributions in binary form must
reproduce the above copyright notice in the documentation and/or other materials
provided with the distribution. The name of W. Randolph Franklin may not be used
to endorse or promote products derived from this Software without specific prior
written permission. THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY
KIND, EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF
MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO
EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR
OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING
FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN
THE SOFTWARE.
*/
__device__ __forceinline__ int pnpoly(int nvert, double2 *vert, double testx,
                                      double testy) {
  int i, j, c = 0;
  for (i = 0, j = nvert - 1; i < nvert; j = i++) {
    if (((vert[i].y > testy) != (vert[j].y > testy)) &&
        (testx < (vert[j].x - vert[i].x) * (testy - vert[i].y) /
                         (vert[j].y - vert[i].y) +
                     vert[i].x))
      c = !c;
  }
  return c;
}

__device__ __forceinline__ int pnpoly(int nvert, float2 *vert, float testx,
                                      float testy) {
  int i, j, c = 0;
  for (i = 0, j = nvert - 1; i < nvert; j = i++) {
    if (((vert[i].y > testy) != (vert[j].y > testy)) &&
        (testx < (vert[j].x - vert[i].x) * (testy - vert[i].y) /
                         (vert[j].y - vert[i].y) +
                     vert[i].x))
      c = !c;
  }
  return c;
}

extern "C" __forceinline__ __device__ void
rtspatial_handle_point_contains(uint32_t geom_id, uint32_t query_id,
                                void *arg) {
  PIPContext *ctx = static_cast<PIPContext *>(arg);
  auto x = ctx->points[query_id].get_x();
  auto y = ctx->points[query_id].get_y();

  auto begin = ctx->row_offsets[geom_id];
  auto end = ctx->row_offsets[geom_id + 1];
  auto nvert = end - begin;


  if (pnpoly(nvert, ctx->vertices.data() + begin, x, y)) {
    if(query_id == 7311) {
      printf("box id %u\n", geom_id);
    }

    ctx->results.Append(thrust::pair<uint32_t, uint32_t>(geom_id, query_id));
  }
}

extern "C" __forceinline__ __device__ void
rtspatial_handle_envelope_contains(uint32_t geom_id, uint32_t query_id,
                                   void *arg) {}

extern "C" __forceinline__ __device__ void
rtspatial_handle_envelope_intersects(uint32_t geom_id, uint32_t query_id,
                                     void *arg) {}
