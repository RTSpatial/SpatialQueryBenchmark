#include "query/rtspatial/common.h"
#include "range_query.h"
#include "rtspatial/rtspatial.h"
#include "stopwatch.h"

time_stat RunRangeQueryRTSpatial(const std::vector<box_t> &boxes,
                                 const std::vector<box_t> &queries,
                                 const BenchmarkConfig &config) {
  rtspatial::Stream stream;
  rtspatial::SpatialIndex<coord_t, 2> index;
  thrust::device_vector<rtspatial::Envelope<rtspatial::Point<coord_t, 2>>>
      d_boxes, d_queries;
  rtspatial::Config idx_config;

  idx_config.ptx_root = std::string(RTSPATIAL_PTX_DIR);
  idx_config.intersect_cost_weight = 0.90;
  idx_config.max_geometries = boxes.size();
  idx_config.compact = false;

  CopyBoxes(boxes, d_boxes);
  CopyBoxes(queries, d_queries);

  index.Init(idx_config);
  time_stat ts;
  Stopwatch sw;

  ts.num_geoms = boxes.size();
  ts.num_queries = queries.size();
  auto queue_size = std::max(
      1ul, (size_t)(boxes.size() * queries.size() * config.load_factor));

  std::cout << "Queue size "
            << queue_size * sizeof(thrust::pair<uint32_t, uint32_t>) / 1024 /
                   1024
            << " MB" << std::endl;

  rtspatial::Queue<thrust::pair<uint32_t, uint32_t>> results;
  rtspatial::SharedValue<
      rtspatial::Queue<thrust::pair<uint32_t, uint32_t>>::device_t>
      d_results;

  results.Init(queue_size);
  d_results.set(stream.cuda_stream(), results.DeviceObject());

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

  index.PrintMemoryUsage();

  auto updates = GenerateUpdates(boxes, config.update_ratio);

  auto run_queries = [&](std::vector<double> &running_times) {
    for (int i = 0; i < config.warmup + config.repeat; i++) {
      results.Clear(stream.cuda_stream());
      sw.start();
      switch (config.query_type) {
      case BenchmarkConfig::QueryType::kRangeContains: {
        index.Query(rtspatial::Predicate::kContains, d_queries,
                    d_results.data(), stream.cuda_stream());
        break;
      }
      case BenchmarkConfig::QueryType::kRangeIntersects: {
        index.Query(rtspatial::Predicate::kIntersects, d_queries,
                    d_results.data(), stream.cuda_stream());
        break;
      }
      default:
        abort();
      }
      // Implicit barrier
      ts.num_results = results.size(stream.cuda_stream());
      sw.stop();
      running_times.push_back(sw.ms());
    }
  };

  if (!updates.empty()) {
    index.Update(
        rtspatial::ArrayView<thrust::pair<
            size_t, rtspatial::Envelope<rtspatial::Point<coord_t, 2>>>>(
            updates),
        stream.cuda_stream());
    stream.Sync();

    // Run Query after updates
    run_queries(ts.query_ms_after_update);

    UpdateBoxes(d_boxes, updates);
    // Rebuild Index on updated geometries
    index.Clear();
    index.Insert(
        rtspatial::ArrayView<rtspatial::Envelope<rtspatial::Point<coord_t, 2>>>(
            d_boxes),
        stream.cuda_stream());
  }

  run_queries(ts.query_ms);

  return ts;
}

time_stat
RunRangeQueryRTSpatialVaryParallelism(const std::vector<box_t> &boxes,
                                      const std::vector<box_t> &queries,
                                      const BenchmarkConfig &config) {
  rtspatial::Stream stream;
  rtspatial::SpatialIndex<coord_t, 2> index;
  thrust::device_vector<rtspatial::Envelope<rtspatial::Point<coord_t, 2>>>
      d_boxes, d_queries;
  rtspatial::Config idx_config;

  idx_config.ptx_root = std::string(RTSPATIAL_PTX_DIR);
  idx_config.intersect_cost_weight = 0.90;
  idx_config.prefer_fast_build_query = false;

  CopyBoxes(boxes, d_boxes);
  CopyBoxes(queries, d_queries);

  index.Init(idx_config);
  time_stat ts;
  Stopwatch sw;

  ts.num_geoms = boxes.size();
  ts.num_queries = queries.size();

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
      1ul, (size_t)(boxes.size() * queries.size() * config.load_factor)));
  d_results.set(stream.cuda_stream(), results.DeviceObject());

  for (int i = 1; i <= config.parallelism; i *= 2) {
    results.Clear(stream.cuda_stream());
    sw.start();
    index.IntersectsWhatQueryProfiling(d_queries, d_results.data(),
                                       stream.cuda_stream(), i);
    ts.num_results = results.size(stream.cuda_stream());
    sw.stop();
    ts.query_ms.push_back(sw.ms());
  }

  sw.start();
  int pred = index.CalculateBestParallelism(d_queries, stream.cuda_stream());
  sw.stop();
  std::cout << "Predicated Parallelism " << pred << " Time " << sw.ms() << " ms"
            << std::endl;

  results.Clear(stream.cuda_stream());

  index.IntersectsWhatQueryProfiling(d_queries, d_results.data(),
                                     stream.cuda_stream(), pred);

  return ts;
}