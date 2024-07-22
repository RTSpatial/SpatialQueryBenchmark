#ifndef SPATIALQUERYBENCHMARK_QUERY_RTSPATIAL_COMMON_H
#define SPATIALQUERYBENCHMARK_QUERY_RTSPATIAL_COMMON_H
#include "geom_common.h"
#include "rtspatial/rtspatial.h"
#include <vector>

inline void CopyBoxes(
    const std::vector<box_t> &boxes,
    thrust::device_vector<rtspatial::Envelope<rtspatial::Point<coord_t, 2>>>
        &d_boxes) {
  pinned_vector<rtspatial::Envelope<rtspatial::Point<coord_t, 2>>> h_boxes;

  h_boxes.resize(boxes.size());

  for (size_t i = 0; i < boxes.size(); i++) {
    rtspatial::Point<coord_t, 2> p_min(boxes[i].min_corner().x(),
                                       boxes[i].min_corner().y());
    rtspatial::Point<coord_t, 2> p_max(boxes[i].max_corner().x(),
                                       boxes[i].max_corner().y());

    h_boxes[i] =
        rtspatial::Envelope<rtspatial::Point<coord_t, 2>>(p_min, p_max);
  }

  d_boxes = h_boxes;
}

inline void
CopyPoints(const std::vector<point_t> &points,
           thrust::device_vector<rtspatial::Point<coord_t, 2>> &d_points) {
  pinned_vector<rtspatial::Point<coord_t, 2>> h_points;

  h_points.resize(points.size());

  for (size_t i = 0; i < points.size(); i++) {
    h_points[i] = rtspatial::Point<coord_t, 2>(points[i].x(), points[i].y());
  }

  d_points = h_points;
}
#endif // SPATIALQUERYBENCHMARK_QUERY_RTSPATIAL_COMMON_H
