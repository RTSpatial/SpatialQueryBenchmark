#ifndef SPATIALQUERYBENCHMARK_QUERY_RTSPATIAL_COMMON_H
#define SPATIALQUERYBENCHMARK_QUERY_RTSPATIAL_COMMON_H
#include "geom_common.h"

#include "rtspatial/rtspatial.h"
#include <random>
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

inline thrust::device_vector<
    thrust::pair<size_t, rtspatial::Envelope<rtspatial::Point<coord_t, 2>>>>
GenerateUpdates(const std::vector<box_t> &boxes, float update_ratio) {
  size_t n_updates = boxes.size() * update_ratio;
  if (n_updates == 0) {
    return {};
  }
  std::vector<size_t> updated_ids(boxes.size());
  coord_t g_min_x = std::numeric_limits<coord_t>::max(), g_min_y = g_min_x;
  coord_t g_max_x = std::numeric_limits<coord_t>::lowest(), g_max_y = g_min_x;

  for (size_t i = 0; i < boxes.size(); i++) {
    updated_ids[i] = i;
    auto &box = boxes[i];
    g_min_x = std::min(g_min_x, box.min_corner().x());
    g_max_x = std::max(g_max_x, box.max_corner().x());
    g_min_y = std::min(g_min_y, box.min_corner().y());
    g_max_y = std::max(g_max_y, box.max_corner().y());
  }

  std::mt19937 g(boxes.size());

  std::shuffle(updated_ids.begin(), updated_ids.end(), g);

  updated_ids.resize(n_updates);

  std::vector<
      thrust::pair<size_t, rtspatial::Envelope<rtspatial::Point<coord_t, 2>>>>
      h_updates;

  for (auto box_id : updated_ids) {
    auto &box = boxes[box_id];
    auto width = box.max_corner().x() - box.min_corner().x();
    auto height = box.max_corner().y() - box.min_corner().x();

    int op = box_id % 3;
    rtspatial::Envelope<rtspatial::Point<coord_t, 2>> new_box;

    if (op == 0) { // Move box
      auto center_x = (box.min_corner().x() + box.max_corner().x()) / 2;
      auto center_y = (box.min_corner().y() + box.max_corner().y()) / 2;
      // Move range will not exceed 10x of the extends of the box
      std::uniform_real_distribution<float> dist_x(center_x - 5 * width,
                                                   center_x + 5 * width);
      std::uniform_real_distribution<float> dist_y(center_y - 5 * height,
                                                   center_y + 5 * height);

      auto min_x = dist_x(g), max_x = min_x + width;
      auto min_y = dist_y(g), max_y = min_y + height;

      new_box = rtspatial::Envelope<rtspatial::Point<coord_t, 2>>(
          rtspatial::Point<coord_t, 2>(min_x, min_y),
          rtspatial::Point<coord_t, 2>(max_x, max_y));

    } else if (op == 1) { // enlarge
      std::uniform_real_distribution<float> dist_width(width, width * 10);
      std::uniform_real_distribution<float> dist_height(height, height * 10);
      auto min_x = box.min_corner().x(), max_x = min_x + dist_width(g);
      auto min_y = box.min_corner().y(), max_y = min_y + dist_height(g);

      new_box = rtspatial::Envelope<rtspatial::Point<coord_t, 2>>(
          rtspatial::Point<coord_t, 2>(min_x, min_y),
          rtspatial::Point<coord_t, 2>(max_x, max_y));
    } else { // shrink
      std::uniform_real_distribution<float> dist_width(0, width);
      std::uniform_real_distribution<float> dist_height(0, height);
      auto min_x = box.min_corner().x(), max_x = min_x + dist_width(g);
      auto min_y = box.min_corner().y(), max_y = min_y + dist_height(g);

      new_box = rtspatial::Envelope<rtspatial::Point<coord_t, 2>>(
          rtspatial::Point<coord_t, 2>(min_x, min_y),
          rtspatial::Point<coord_t, 2>(max_x, max_y));
    }

    h_updates.push_back(thrust::make_pair(box_id, new_box));
  }

  thrust::device_vector<
      thrust::pair<size_t, rtspatial::Envelope<rtspatial::Point<coord_t, 2>>>>
      d_updates = h_updates;
  return d_updates;
}

inline void UpdateBoxes(
    thrust::device_vector<rtspatial::Envelope<rtspatial::Point<coord_t, 2>>>
        &d_boxes,
    const thrust::device_vector<
        thrust::pair<size_t, rtspatial::Envelope<rtspatial::Point<coord_t, 2>>>>
        &updates) {
  auto *p_boxes = thrust::raw_pointer_cast(d_boxes.data());

  thrust::for_each(
      thrust::device, updates.begin(), updates.end(),
      [=] __device__(
          const thrust::pair<size_t,
                             rtspatial::Envelope<rtspatial::Point<coord_t, 2>>>
              &update) { p_boxes[update.first] = update.second; });
}

#endif // SPATIALQUERYBENCHMARK_QUERY_RTSPATIAL_COMMON_H
