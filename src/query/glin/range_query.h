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

  auto *p_boxes = &boxes;
  auto *p_queries = &queries;
  bool piece = true;

  if (config.query_type == BenchmarkConfig::QueryType::kRangeContains) {
    piece = false;
    std::swap(p_boxes, p_queries);
  }

  alex::Glin<double, geos::geom::Geometry *> index(piece);
  std::vector<std::tuple<double, double, double, double>> pieces;
  std::vector<std::unique_ptr<geos::geom::Geometry>> geoms_vec;
  std::vector<geos::geom::Geometry *> geoms_ptrs;
  std::unique_ptr<geos::geom::PrecisionModel> pm(
      new geos::geom::PrecisionModel());
  geos::geom::GeometryFactory::Ptr global_factory =
      geos::geom::GeometryFactory::create(pm.get(), -1);
  double piece_limitation = 10000; // a good value suggested in their paper

  // GLIN's definition of "Contains" is different from the other libraries
  // So we need to swap queries and boxes

  for (auto &box : *p_boxes) {
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

#ifdef COLLECT_RESULTS
  std::vector<geos::geom::Geometry *> results;
  std::mutex mu;
#endif

  for (int i = 0; i < config.warmup + config.repeat; i++) {
    size_t avg_queries =
        (p_queries->size() + config.parallelism - 1) / config.parallelism;
    std::vector<std::thread> threads;
    std::atomic_uint64_t total_num_results;

    sw.start();
    ts.num_results = 0;
    total_num_results = 0;
#ifdef COLLECT_RESULTS
    results.clear();
#endif
    for (int tid = 0; tid < config.parallelism; tid++) {
      threads.emplace_back(
          [&](int tid) {
            auto begin = std::min(tid * avg_queries, p_queries->size());
            auto end = std::min(begin + avg_queries, p_queries->size());
            std::vector<geos::geom::Geometry *> local_results;

            for (auto i = begin; i < end; i++) {
              auto &q = (*p_queries)[i];
              geos::geom::Envelope env(q.min_corner().x(), q.max_corner().x(),
                                       q.min_corner().y(), q.max_corner().y());
              auto bbox = global_factory->toGeometry(&env)->clone();
              int count_filter = 0;
              std::vector<geos::geom::Geometry *> find_result;

              switch (config.query_type) {
              case BenchmarkConfig::QueryType::kRangeContains:
              case BenchmarkConfig::QueryType::kRangeIntersects:
                index.glin_find(bbox.get(), "z", cell_xmin, cell_ymin,
                                cell_x_intvl, cell_y_intvl, pieces, find_result,
                                count_filter);
                break;
              default:
                abort();
              }
#ifdef COLLECT_RESULTS
              local_results.insert(local_results.end(), find_result.begin(),
                                   find_result.end());
#endif

#ifdef COUNT_RESULTS
              total_num_results += find_result.size();
#endif
            }

#ifdef COLLECT_RESULTS
            std::unique_lock<std::mutex> lock(mu);
            results.insert(results.end(), local_results.begin(),
                           local_results.end());
#endif
          },
          tid);
    }
    for (auto &thread : threads) {
      thread.join();
    }
#ifdef COLLECT_RESULTS
    ts.num_results = results.size();
#endif

#ifdef COUNT_RESULTS
    ts.num_results = total_num_results;
#endif
    sw.stop();
    ts.query_ms.push_back(sw.ms());
  }

  return ts;
}
#endif // SPATIALQUERYBENCHMARK_GLIN_RANGE_QUERY_H
