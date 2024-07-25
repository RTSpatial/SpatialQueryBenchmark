#ifndef SPATIALQUERYBENCHMARK_GLIN_RANGE_QUERY_H
#define SPATIALQUERYBENCHMARK_GLIN_RANGE_QUERY_H
#include "stopwatch.h"
#include "time_stat.h"
#include <mutex>
#include <thread>

#include "glin/glin.h"

time_stat RunRangeQueryGLIN(const std::vector<box_t> &boxes,
                            const std::vector<box_t> &queries,
                            const BenchmarkConfig &config) {
  Stopwatch sw;
  time_stat ts;

  ts.num_geoms = boxes.size();
  ts.num_queries = queries.size();

  alex::Glin<float, geos::geom::Geometry *> index;
  std::vector<std::tuple<double, double, double, double>> pieces;
  std::vector<std::unique_ptr<geos::geom::Geometry>> geoms_vec;
  std::vector<geos::geom::Geometry *> geoms_ptrs;
  std::unique_ptr<geos::geom::PrecisionModel> pm(
      new geos::geom::PrecisionModel());
  geos::geom::GeometryFactory::Ptr global_factory =
      geos::geom::GeometryFactory::create(pm.get(), -1);
  double piece_limitation = 10000; // a good value suggested in their paper

  for (auto &box : boxes) {
    geos::geom::Envelope env(box.min_corner().x(), box.max_corner().x(),
                             box.min_corner().y(), box.max_corner().y());
    auto bbox = global_factory->toGeometry(&env)->clone();

    geoms_vec.emplace_back(std::move(bbox));
    geoms_ptrs.push_back(geoms_vec.back().get());
  }

  double cell_xmin = -180;
  double cell_ymin = -180;
  double cell_x_intvl = 0.0000005;
  double cell_y_intvl = 0.0000005;

  for (int i = 0; i < config.warmup + config.repeat; i++) {
    index.clear();
    sw.start();
    index.glin_bulk_load(geoms_ptrs, piece_limitation, "z", cell_xmin,
                         cell_ymin, cell_x_intvl, cell_y_intvl, pieces);
    sw.stop();
    ts.insert_ms.push_back(sw.ms());
  }

  std::vector<geos::geom::Geometry *> find_result;

  for (int i = 0; i < config.warmup + config.repeat; i++) {
    //    results.clear();
    sw.start();
    ts.num_results = 0;
    for (auto &q : queries) {
      geos::geom::Envelope env(q.min_corner().x(), q.max_corner().x(),
                               q.min_corner().y(), q.max_corner().y());
      auto bbox = global_factory->toGeometry(&env)->clone();
      int count_filter = 0;
      find_result.clear();

      switch (config.query_type) {
      case BenchmarkConfig::QueryType::kRangeContains: {
        break;
      }
      case BenchmarkConfig::QueryType::kRangeIntersects: {
        index.glin_find(bbox.get(), "z", cell_xmin, cell_ymin, cell_x_intvl,
                        cell_y_intvl, pieces, find_result, count_filter);
        break;
      }
      default:
        abort();
      }
      ts.num_results += find_result.size();
    }
    sw.stop();
    ts.query_ms.push_back(sw.ms());
  }

  return ts;
}
#endif // SPATIALQUERYBENCHMARK_BOOST_RANGE_QUERY_H
