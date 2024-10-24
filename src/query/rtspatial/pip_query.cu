#include "cdb_loader.h"
#include "query/rtspatial/common.h"
#include "query/rtspatial/pip_context.h"
#include "query/rtspatial/pip_query.h"
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

time_stat RunPIPQueryRTSpatial(const std::vector<polygon_t> &polygons,
                               const std::vector<point_t> &points,
                               const BenchmarkConfig &config) {
  std::vector<rtspatial::Envelope<rtspatial::Point<coord_t, 2>>> boxes(
      polygons.size());
  std::vector<rtspatial::Point<coord_t, 2>> queries(points.size());
  std::vector<uint32_t> row_offsets;
  std::vector<float2> vertices;
  uint32_t tail = 0;

  row_offsets.push_back(tail);

  for (size_t i = 0; i < boxes.size(); i++) {
    const auto &polygon = polygons[i];
    coord_t lows[2] = {std::numeric_limits<coord_t>::max(),
                       std::numeric_limits<coord_t>::max()};
    coord_t highs[2] = {std::numeric_limits<coord_t>::lowest(),
                        std::numeric_limits<coord_t>::lowest()};

    for (auto &p : polygon.outer()) {
      lows[0] = std::min(lows[0], p.x());
      highs[0] = std::max(highs[0], p.x());
      lows[1] = std::min(lows[1], p.y());
      highs[1] = std::max(highs[1], p.y());
    }

    rtspatial::Envelope<rtspatial::Point<coord_t, 2>> envelope(
        rtspatial::Point<coord_t, 2>(lows[0], lows[1]),
        rtspatial::Point<coord_t, 2>(highs[0], highs[1]));

    boxes[i] = envelope;

    // https://wrfranklin.org/Research/Short_Notes/pnpoly.html
    vertices.push_back(float2{0, 0});
    tail++;

    for (auto &p : polygon.outer()) {
      vertices.push_back(float2{p.x(), p.y()});
      tail++;
    }
    vertices.push_back(float2{0, 0});
    tail++;

    // fill holes
    for (auto &inner : polygon.inners()) {
      for (auto &p : inner) {
        vertices.push_back(float2{p.x(), p.y()});
        tail++;
      }
      vertices.push_back(float2{0, 0});
      tail++;
    }
    row_offsets.push_back(tail);
  }

  for (size_t i = 0; i < points.size(); i++) {
    queries[i].set_x(points[i].x());
    queries[i].set_y(points[i].y());
  }

  thrust::device_vector<rtspatial::Envelope<rtspatial::Point<coord_t, 2>>>
      d_boxes = boxes;
  thrust::device_vector<rtspatial::Point<coord_t, 2>> d_queries = queries;
  thrust::device_vector<uint32_t> d_row_offsets = row_offsets;
  thrust::device_vector<float2> d_vertices = vertices;

  rtspatial::Stream stream;
  rtspatial::SpatialIndex<float, 2> index;
  rtspatial::Config idx_config;

  idx_config.max_geometries = d_boxes.size();
  idx_config.max_queries = d_queries.size();
  idx_config.ptx_root = std::string(RTSPATIAL_PTX_DIR);

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

  rtspatial::Queue<thrust::pair<uint32_t, uint32_t>> results;
  PIPContext ctx;
  rtspatial::SharedValue<PIPContext> d_ctx;

  results.Init(std::max(
      1ul, (size_t)(ts.num_geoms * ts.num_queries * config.load_factor)));
  ctx.row_offsets = d_row_offsets;
  ctx.vertices = d_vertices;
  ctx.points = d_queries;
  ctx.results = results.DeviceObject();

  d_ctx.set(stream.cuda_stream(), ctx);

  for (int i = 0; i < config.warmup + config.repeat; i++) {
    results.Clear(stream.cuda_stream());
    sw.start();
    index.Query(rtspatial::Predicate::kContains, d_queries, d_ctx.data(),
                stream.cuda_stream());
    ts.num_results = results.size(stream.cuda_stream());
    sw.stop();
    ts.query_ms.push_back(sw.ms());
  }

  return ts;
}
