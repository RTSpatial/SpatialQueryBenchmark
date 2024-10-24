#ifndef SPATIALQUERYBENCHMARK_PARGEO_POINT_QUERY_H
#define SPATIALQUERYBENCHMARK_PARGEO_POINT_QUERY_H
#include "stopwatch.h"
#include "time_stat.h"
#include "wkt_loader.h"

#include "kdTree/kdTree.h"
#include "pargeo/point.h"

#include <mutex>
#include <thread>

time_stat RunPointQueryParGeo(const std::vector<box_t> &boxes,
                              const std::vector<point_t> &queries,
                              const BenchmarkConfig &config) {
  using pargeo_point_t = pargeo::fpoint<2>;
  using node_t = pargeo::kdTree::node<2, pargeo_point_t>;
  Stopwatch sw;
  time_stat ts;

  parlay::sequence<pargeo_point_t> points(queries.size());

  std::cout << "num_workers " << parlay::num_workers() << std::endl;

  for (size_t i = 0; i < queries.size(); i++) {
    points[i].x[0] = queries[i].x();
    points[i].x[1] = queries[i].y();
  }

  ts.num_geoms = boxes.size();
  ts.num_queries = queries.size();

  node_t *tree = nullptr;
  std::vector<pargeo_point_t *> results;
  std::mutex mu;

  for (int i = 0; i < config.warmup + config.repeat; i++) {
    if (tree != nullptr) {
      pargeo::kdTree::del(tree);
    }

    sw.start();
    tree = pargeo::kdTree::build<2, pargeo_point_t>(points, true);
    sw.stop();
    std::cout << sw.ms() << std::endl;
    ts.insert_ms.push_back(sw.ms());
  }

  for (int i = 0; i < config.warmup + config.repeat; i++) {
    std::vector<std::thread> threads;
    size_t avg_queries =
        (boxes.size() + config.parallelism - 1) / config.parallelism;

    sw.start();
    ts.num_results = 0;
    results.clear();

    for (int tid = 0; tid < config.parallelism; tid++) {
      threads.emplace_back(
          [&](int tid) {
            auto begin = std::min(tid * avg_queries, boxes.size());
            auto end = std::min(begin + avg_queries, boxes.size());
            size_t num_results = 0;
            std::vector<pargeo_point_t *> local_results;

            for (auto i = begin; i < end; i++) {
              auto &p = boxes[i];

              pargeo_point_t p_min, p_max;
              p_min.x[0] = p.min_corner().x();
              p_min.x[1] = p.min_corner().y();
              p_max.x[0] = p.max_corner().x();
              p_max.x[1] = p.max_corner().y();

              auto callback = [&](pargeo_point_t *p) {
                local_results.push_back(p);
              };

              pargeo::kdTree::orthRangeHelper<2, node_t, pargeo_point_t,
                                              decltype(callback)>(
                  tree, p_min, p_max, callback);
            }

            std::unique_lock<std::mutex> lock(mu);
            results.insert(results.end(), local_results.begin(),
                           local_results.end());
          },
          tid);
    }
    for (auto &thread : threads) {
      thread.join();
    }
    ts.num_results = results.size();
    sw.stop();
    ts.query_ms.push_back(sw.ms());
  }
  pargeo::kdTree::del(tree);
  return ts;
}

#endif // SPATIALQUERYBENCHMARK_PARGEO_POINT_QUERY_H
