#ifndef SPATIALQUERYBENCHMARK_BOOST_POINT_QUERY_H
#define SPATIALQUERYBENCHMARK_BOOST_POINT_QUERY_H
#include "stopwatch.h"
#include "time_stat.h"
#include "wkt_loader.h"
#include <boost/iterator/function_output_iterator.hpp>
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

  for (int i = 0; i < config.warmup + config.repeat; i++) {
    rtree.clear();
    sw.start();
    rtree.insert(boxes);
    sw.stop();
    ts.insert_ms.push_back(sw.ms());
  }

  for (int i = 0; i < config.warmup + config.repeat; i++) {
    ts.num_results = 0;
    sw.start();
    for (auto &p : queries) {
      rtree.query(boost::geometry::index::contains(p),
                  boost::make_function_output_iterator(
                      [&](const box_t &b) { ts.num_results++; }));
    }
    sw.stop();
    ts.query_ms.push_back(sw.ms());
  }

  return ts;
}

time_stat RunParallelPointQueryBoost(const std::vector<box_t> &boxes,
                                     const std::vector<point_t> &queries,
                                     const BenchmarkConfig &config) {
  Stopwatch sw;
  time_stat ts;

  ts.num_geoms = boxes.size();
  ts.num_queries = queries.size();

  boost::geometry::index::rtree<box_t, boost::geometry::index::rstar<16>,
                                boost::geometry::index::indexable<box_t>>
      rtree;

  for (int i = 0; i < config.warmup + config.repeat; i++) {
    rtree.clear();
    sw.start();
    rtree.insert(boxes);
    sw.stop();
    ts.insert_ms.push_back(sw.ms());
  }

  for (int i = 0; i < config.warmup + config.repeat; i++) {
    std::vector<std::thread> threads;
    std::atomic_uint64_t total_num_results;
    size_t avg_queries =
        (ts.num_queries + config.parallelism - 1) / config.parallelism;

    sw.start();
    ts.num_results = 0;
    total_num_results = 0;
    for (int tid = 0; tid < config.parallelism; tid++) {
      threads.emplace_back(
          [&](int tid) {
            auto begin = std::min(tid * avg_queries, ts.num_queries);
            auto end = std::min(begin + avg_queries, ts.num_queries);
            size_t num_results = 0;

            for (auto i = begin; i < end; i++) {
              auto &p = queries[i];
              rtree.query(boost::geometry::index::contains(p),
                          boost::make_function_output_iterator(
                              [&](const box_t &b) { num_results++; }));
            }
            total_num_results += num_results;
          },
          tid);
    }
    for (auto &thread : threads) {
      thread.join();
    }
    ts.num_results = total_num_results;
    sw.stop();
    ts.query_ms.push_back(sw.ms());
  }
  return ts;
}

#endif // SPATIALQUERYBENCHMARK_BOOST_POINT_QUERY_H
