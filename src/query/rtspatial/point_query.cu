#include "point_query.h"
#include "query/rtspatial/common.h"
#include "rtspatial/rtspatial.h"
#include "stopwatch.h"
time_stat RunPointQueryRTSpatial(const std::vector<box_t> &boxes,
                                 const std::vector<point_t> &queries,
                                 const BenchmarkConfig &config) {
  rtspatial::Stream stream;
  rtspatial::SpatialIndex<coord_t, 2> index;
  thrust::device_vector<rtspatial::Envelope<rtspatial::Point<coord_t, 2>>>
      d_boxes;
  thrust::device_vector<rtspatial::Point<coord_t, 2>> d_queries;
  rtspatial::Config idx_config;

  idx_config.ptx_root = std::string(RTSPATIAL_PTX_DIR);
  idx_config.max_geometries = boxes.size();

  CopyBoxes(boxes, d_boxes);
  CopyPoints(queries, d_queries);

  index.Init(idx_config);
  time_stat ts;
  Stopwatch sw;

  ts.num_geoms = boxes.size();
  ts.num_queries = queries.size();

  rtspatial::Queue<thrust::pair<uint32_t, uint32_t>> results;
  rtspatial::SharedValue<
      rtspatial::Queue<thrust::pair<uint32_t, uint32_t>>::device_t>
      d_results;

  results.Init(std::max(
      1ul, (size_t)(boxes.size() * queries.size() * config.load_factor)));
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

  auto updates = GenerateUpdates(boxes, config.update_ratio);

  auto run_queries = [&](std::vector<double> &running_times) {
    for (int i = 0; i < config.warmup + config.repeat; i++) {
      results.Clear(stream.cuda_stream());
      sw.start();
      switch (config.query_type) {
      case BenchmarkConfig::QueryType::kPointContains: {
        index.Query(rtspatial::Predicate::kContains, d_queries,
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