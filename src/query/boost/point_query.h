#ifndef SPATIALQUERYBENCHMARK_BOOST_POINT_QUERY_H
#define SPATIALQUERYBENCHMARK_BOOST_POINT_QUERY_H
#include "stopwatch.h"
#include "time_stat.h"
#include "wkt_loader.h"
#include <mutex>
#include <thread>

time_stat RunPointQueryBoost(const std::vector<box_t> &boxes,
                             const std::vector<point_t> &queries,
                             const BenchmarkConfig &config) {
  Stopwatch sw;
  time_stat ts;

  ts.num_geoms = boxes.size();
  ts.num_queries = queries.size();

  boost::geometry::index::rtree<box_t, boost::geometry::index::rstar<16>,
                                boost::geometry::index::indexable<box_t>>
      rtree;
  std::vector<box_t> results;

  results.reserve(std::max(
      1ul, size_t(boxes.size() * queries.size() * config.load_factor)));

  for (int i = 0; i < config.warmup + config.repeat; i++) {
    rtree.clear();
    sw.start();
    rtree.insert(boxes);
    sw.stop();
    ts.insert_ms.push_back(sw.ms());
  }

  for (int i = 0; i < config.warmup + config.repeat; i++) {
    results.clear();
    sw.start();
    for (auto &p : queries) {
      rtree.query(boost::geometry::index::contains(p),
                  std::back_inserter(results));
    }
    sw.stop();
    ts.query_ms.push_back(sw.ms());
  }
  ts.num_results = results.size();

  return ts;
}

time_stat RunParallelPointQueryBoost(const std::vector<box_t> &boxes,
                                     const std::vector<point_t> &queries,
                                     const BenchmarkConfig &config) {
  Stopwatch sw;
  time_stat ts;

  ts.num_geoms = boxes.size();
  ts.num_queries = queries.size();
  ts.num_threads = std::thread::hardware_concurrency();

  boost::geometry::index::rtree<box_t, boost::geometry::index::rstar<16>,
                                boost::geometry::index::indexable<box_t>>
      rtree;
  std::vector<box_t> results;

  results.reserve(std::max(
      1ul, size_t(boxes.size() * queries.size() * config.load_factor)));

  for (int i = 0; i < config.warmup + config.repeat; i++) {
    rtree.clear();
    sw.start();
    rtree.insert(boxes);
    sw.stop();
    ts.insert_ms.push_back(sw.ms());
  }

  for (int i = 0; i < config.warmup + config.repeat; i++) {
    std::vector<std::thread> threads;
    std::mutex mu;
    results.clear();

    size_t avg_queries = (ts.num_queries + ts.num_threads - 1) / ts.num_threads;
    sw.start();
    for (size_t tid = 0; tid < ts.num_threads; tid++) {
      threads.emplace_back(
          [&](size_t tid) {
            auto begin = std::min(tid * avg_queries, ts.num_queries);
            auto end = std::min(begin + avg_queries, ts.num_queries);
            std::vector<box_t> local_results;

            for (auto i = begin; i < end; i++) {
              auto &p = queries[i];
              rtree.query(boost::geometry::index::contains(p),
                          std::back_inserter(local_results));
            }
            {
              std::unique_lock<std::mutex> lock(mu);
              results.insert(results.end(), local_results.begin(),
                             local_results.end());
            }
          },
          tid);
    }
    for (auto &thread : threads) {
      thread.join();
    }
    sw.stop();
    ts.query_ms.push_back(sw.ms());
  }

  ts.num_results = results.size();

  return ts;
}

#endif // SPATIALQUERYBENCHMARK_BOOST_POINT_QUERY_H
