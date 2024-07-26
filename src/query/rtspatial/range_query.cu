#include "query/rtspatial/common.h"
#include "range_query.h"
#include "rtspatial/rtspatial.h"
#include "stopwatch.h"

time_stat RunRangeQueryRTSpatial(const std::vector<box_t> &boxes,
                                 const std::vector<box_t> &queries,
                                 const BenchmarkConfig &config) {
  rtspatial::Stream stream;
  rtspatial::SpatialIndex<coord_t, 2, false> index;
  thrust::device_vector<rtspatial::Envelope<rtspatial::Point<coord_t, 2>>>
      d_boxes, d_queries;
  rtspatial::Config idx_config;

  idx_config.ptx_root = std::string(RTSPATIAL_PTX_DIR);

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
    index.Insert(d_boxes, stream.cuda_stream());
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

  for (int i = 0; i < config.warmup + config.repeat; i++) {
    results.Clear(stream.cuda_stream());
    sw.start();
    switch (config.query_type) {
    case BenchmarkConfig::QueryType::kRangeContains: {
      index.ContainsWhatQuery(d_queries, d_results.data(),
                              stream.cuda_stream());
      break;
    }
    case BenchmarkConfig::QueryType::kRangeIntersects: {
      int best_parallelism =
          index.CalculateBestParallelism(d_queries, stream.cuda_stream());
      index.IntersectsWhatQuery(d_queries, d_results.data(),
                                stream.cuda_stream(), best_parallelism);
      break;
    }
    default:
      abort();
    }
    // Implicit barrier
    ts.num_results = results.size(stream.cuda_stream());
    sw.stop();
    ts.query_ms.push_back(sw.ms());
  }

  return ts;
}

time_stat
RunRangeQueryRTSpatialVaryParallelism(const std::vector<box_t> &boxes,
                                      const std::vector<box_t> &queries,
                                      const BenchmarkConfig &config) {
  rtspatial::Stream stream;
  rtspatial::SpatialIndex<coord_t, 2, false> index;
  thrust::device_vector<rtspatial::Envelope<rtspatial::Point<coord_t, 2>>>
      d_boxes, d_queries;
  rtspatial::Config idx_config;

  idx_config.ptx_root = std::string(RTSPATIAL_PTX_DIR);

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
    index.Insert(d_boxes, stream.cuda_stream());
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
    index.IntersectsWhatQuery(d_queries, d_results.data(), stream.cuda_stream(),
                              i);
    ts.num_results = results.size(stream.cuda_stream());
    sw.stop();
    ts.query_ms.push_back(sw.ms());
  }

  return ts;
}