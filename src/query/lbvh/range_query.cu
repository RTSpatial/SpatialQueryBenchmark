#include "lbvh.cuh"
#include "range_query.h"
#include "rtspatial/utils/queue.h"
#include "stopwatch.h"

struct aabb_getter {
  __device__ lbvh::aabb<float> operator()(float4 &box) const noexcept {
    lbvh::aabb<float> retval;

    retval.lower = make_float4(box.x, box.y, 0, 0);
    retval.upper = make_float4(box.z, box.w, 0, 0);
    return retval;
  }
};

time_stat RunRangeQueryLBVH(const std::vector<box_t> &boxes,
                            const std::vector<box_t> &queries,
                            const BenchmarkConfig &config) {
  std::vector<float4> corners;

  corners.reserve(boxes.size());
  for (auto &box : boxes) {
    corners.push_back(make_float4(box.min_corner().x(), box.min_corner().y(),
                                  box.max_corner().x(), box.max_corner().y()));
  }

  thrust::device_vector<float4> d_boxes(corners);

  corners.clear();
  for (auto &box : queries) {
    corners.push_back(make_float4(box.min_corner().x(), box.min_corner().y(),
                                  box.max_corner().x(), box.max_corner().y()));
  }

  thrust::device_vector<float4> d_queries(corners);
  lbvh::bvh<coord_t, float4, aabb_getter> lbvh;
  time_stat ts;
  Stopwatch sw;

  ts.num_geoms = boxes.size();
  ts.num_queries = queries.size();

  for (int i = 0; i < config.warmup + config.repeat; i++) {
    sw.start();
    lbvh.assign(d_boxes.begin(), d_boxes.end());
    sw.stop();
    ts.insert_ms.push_back(sw.ms());
  }

  auto p_lbvh = lbvh.get_device_repr();

  rtspatial::Queue<thrust::pair<uint32_t, uint32_t>> results;
  results.Init(std::max(
      1ul, (size_t)(boxes.size() * queries.size() * config.load_factor)));
  auto d_results = results.DeviceObject();

  for (int i = 0; i < config.warmup + config.repeat; i++) {
    results.Clear();
    sw.start();
    switch (config.query_type) {
    case BenchmarkConfig::QueryType::kRangeContains: {
      auto *p_boxes = thrust::raw_pointer_cast(d_boxes.data());

      thrust::for_each(
          thrust::make_zip_iterator(thrust::make_tuple(
              thrust::make_counting_iterator<uint32_t>(0), d_queries.begin())),
          thrust::make_zip_iterator(thrust::make_tuple(
              thrust::make_counting_iterator<uint32_t>(d_queries.size()),
              d_queries.end())),
          [=] __device__(const thrust::tuple<uint32_t, float4> &tuple) mutable {
            uint32_t query_id = thrust::get<0>(tuple);
            const auto &query = thrust::get<1>(tuple);

            lbvh::aabb<float> box;

            box.lower = make_float4(query.x, query.y, 0, 0);
            box.upper = make_float4(query.z, query.w, 0, 0);

            lbvh::query_device_all(
                p_lbvh, lbvh::contains(box),
                [=] __device__(std::uint32_t geom_id) mutable {
                  d_results.Append(thrust::make_pair(geom_id, query_id));
                });
          });
      break;
    }
    case BenchmarkConfig::QueryType::kRangeIntersects: {
      thrust::for_each(
          thrust::make_zip_iterator(thrust::make_tuple(
              thrust::make_counting_iterator<uint32_t>(0), d_queries.begin())),
          thrust::make_zip_iterator(thrust::make_tuple(
              thrust::make_counting_iterator<uint32_t>(d_queries.size()),
              d_queries.end())),
          [=] __device__(const thrust::tuple<uint32_t, float4> &tuple) mutable {
            uint32_t query_id = thrust::get<0>(tuple);
            const auto &query = thrust::get<1>(tuple);

            lbvh::aabb<float> box;

            box.lower = make_float4(query.x, query.y, 0, 0);
            box.upper = make_float4(query.z, query.w, 0, 0);

            lbvh::query_device_all(
                p_lbvh, lbvh::overlaps(box),
                [=] __device__(std::uint32_t geom_id) mutable {
                  d_results.Append(thrust::make_pair(geom_id, query_id));
                });
          });
      break;
    }
    default:
      abort();
    }
    // Implicit barrier
    ts.num_results = results.size();
    sw.stop();
    ts.query_ms.push_back(sw.ms());
  }

  return ts;
}
