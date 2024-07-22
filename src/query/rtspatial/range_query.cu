#include "query/rtspatial/common.h"
#include "range_query.h"
#include "rtspatial/rtspatial.h"
#include "stopwatch.h"

time_stat RunRangeQueryRTSpatial(const std::vector<box_t> &boxes,
                                 const std::vector<box_t> &queries,
                                 const BenchmarkConfig &config) {
  rtspatial::Stream stream;
  rtspatial::SpatialIndex<coord_t, 2, false> index;
  rtspatial::Queue<thrust::pair<uint32_t, uint32_t>> results;
  thrust::device_vector<rtspatial::Envelope<rtspatial::Point<coord_t, 2>>>
      d_boxes, d_queries;
  rtspatial::Config idx_config;

  idx_config.ptx_root = std::string(RTSPATIAL_LIBRARY_DIR) + "/ptx";

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
  size_t queue_size = std::max(
      1ul, (size_t)(boxes.size() * queries.size() * config.load_factor));

  std::cout << "Result queue capacity: " << queue_size << ", memory: "
            << queue_size * sizeof(thrust::pair<uint32_t, uint32_t>) / 1024 /
                   1024
            << " MB" << std::endl;
  results.Init(queue_size);
  results.Clear(stream.cuda_stream());

  for (int i = 0; i < config.warmup + config.repeat; i++) {
    results.Clear(stream.cuda_stream());
    sw.start();
    switch (config.query_type) {
    case BenchmarkConfig::QueryType::kRangeContains: {
      index.ContainsWhatQuery(d_queries, results, stream.cuda_stream());
      break;
    }
    case BenchmarkConfig::QueryType::kRangeIntersects: {
      index.IntersectsWhatQuery(d_queries, results, stream.cuda_stream(),
                                FLAGS_rays);
      break;
    }
    default:
      abort();
    }
    ts.num_results = results.size(stream.cuda_stream());
    sw.stop();
    ts.query_ms.push_back(sw.ms());
  }

  return ts;
}