#include "cdb_loader.h"
#include "query/rtspatial/common.h"
#include "query/rtspatial/lsi_query.h"
#include "rtspatial/rtspatial.h"
#include "stopwatch.h"

float next_float_from_double(double v, int dir, int iter = 1) {
  assert(dir == 1 || dir == -1);
  auto fv = static_cast<float>(v); // pos number
  if (fv == 0) {
    return 0.0f;
  }
  float to = v * dir < 0 ? 0 : dir * std::numeric_limits<float>::infinity();

  for (int i = 0; i < iter; i++) {
    fv = std::nextafter(fv, to);
  }

  return fv;
};

time_stat
RunLSIQueryRTSpatial(const std::shared_ptr<PlanarGraph<double>> &pgraph1,
                     const std::shared_ptr<PlanarGraph<double>> &pgraph2,
                     const BenchmarkConfig &config) {
  std::vector<double> points1_x, points1_y;
  std::vector<Edge<double>> edges1;

  std::vector<double> points2_x, points2_y;
  std::vector<Edge<double>> edges2;

  ExtractLineSegs(pgraph1, points1_x, points1_y, edges1);
  ExtractLineSegs(pgraph2, points2_x, points2_y, edges2);

  auto get_boxes = [](const std::vector<double> &points_x,
                      const std::vector<double> &points_y,
                      const std::vector<Edge<double>> &edges) {
    std::vector<rtspatial::Envelope<rtspatial::Point<float, 2>>> boxes;

    boxes.reserve(edges.size());

    for (auto &e : edges) {
      auto min_x = std::min(points_x[e.p1_idx], points_x[e.p2_idx]);
      auto min_y = std::min(points_y[e.p1_idx], points_y[e.p2_idx]);
      auto max_x = std::max(points_x[e.p1_idx], points_x[e.p2_idx]);
      auto max_y = std::max(points_y[e.p1_idx], points_y[e.p2_idx]);

      rtspatial::Envelope<rtspatial::Point<float, 2>> box(
          rtspatial::Point<float, 2>(next_float_from_double(min_x, -1, 2),
                                     next_float_from_double(min_y, -1, 2)),
          rtspatial::Point<float, 2>(next_float_from_double(max_x, 1, 2),
                                     next_float_from_double(max_y, 1, 2)));
      boxes.push_back(box);
    }
    return boxes;
  };
  thrust::device_vector<rtspatial::Envelope<rtspatial::Point<coord_t, 2>>>
      d_boxes = get_boxes(points1_x, points1_y, edges1),
      d_queries = get_boxes(points2_x, points2_y, edges2);

  std::cout << "Loaded\n";

  rtspatial::Stream stream;
  rtspatial::SpatialIndex<float, 2> index;
  rtspatial::Config idx_config;

  idx_config.max_geometries = d_boxes.size();
  idx_config.max_queries = d_queries.size();
  idx_config.ptx_root = std::string(RTSPATIAL_PTX_DIR);
  idx_config.intersect_cost_weight = 0.90;
  idx_config.prefer_fast_build_query = false;

  index.Init(idx_config);
  time_stat ts;
  Stopwatch sw;

  ts.num_geoms = d_boxes.size();
  ts.num_queries = d_queries.size();

  for (int i = 0; i < config.warmup + config.repeat; i++) {
    index.Clear();
    sw.start();
    index.Insert(
        rtspatial::ArrayView<rtspatial::Envelope<rtspatial::Point<coord_t, 2>>>(
            d_boxes),
        stream.cuda_stream());
    stream.Sync();
    sw.stop();
    ts.insert_ms.push_back(sw.ms());
  }

  d_boxes.resize(0);
  d_boxes.shrink_to_fit();

  rtspatial::Queue<thrust::pair<uint32_t, uint32_t>> results;
  rtspatial::SharedValue<
      rtspatial::Queue<thrust::pair<uint32_t, uint32_t>>::device_t>
      d_results;

  results.Init(std::max(
      1ul, (size_t)(ts.num_geoms * ts.num_queries * config.load_factor)));
  d_results.set(stream.cuda_stream(), results.DeviceObject());

  int best_parallelism =
      index.CalculateBestParallelism(d_queries, stream.cuda_stream());

  for (int i = 0; i < config.warmup + config.repeat; i++) {
    results.Clear(stream.cuda_stream());
    sw.start();
    index.IntersectsWhatQuery(d_queries, d_results.data(), stream.cuda_stream(),
                              best_parallelism);
    ts.num_results = results.size(stream.cuda_stream());
    sw.stop();
    std::cout << sw.ms() << std::endl;
    ts.query_ms.push_back(sw.ms());
  }

  return ts;
}
