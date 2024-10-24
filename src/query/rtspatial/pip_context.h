#pragma once
#include "config.h"
#include "rtspatial/geom/point.cuh"
#include "rtspatial/utils/array_view.h"
#include "rtspatial/utils/queue.h"

struct PIPContext {
  rtspatial::ArrayView<uint32_t> row_offsets;
  rtspatial::ArrayView<float2> vertices;

  rtspatial::ArrayView<rtspatial::Point<coord_t, 2>> points;
  rtspatial::dev::Queue<thrust::pair<uint32_t, uint32_t>> results;
};
