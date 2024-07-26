#ifndef SPATIALQUERYBENCHMARK_BOOST_RANGE_QUERY_H
#define SPATIALQUERYBENCHMARK_BOOST_RANGE_QUERY_H
#include "stopwatch.h"
#include "time_stat.h"
#include <mutex>
#include <thread>

time_stat RunRangeQueryBoost(const std::vector<box_t> &boxes,
                             const std::vector<box_t> &queries,
                             const BenchmarkConfig &config) {
  Stopwatch sw;
  time_stat ts;

  ts.num_geoms = boxes.size();
  ts.num_queries = queries.size();

  boost::geometry::index::rtree<box_t,
                                boost::geometry::index::linear<BOOST_LEAF_SIZE>,
                                boost::geometry::index::indexable<box_t>>
      rtree;
  std::vector<box_t> results;
  std::mutex mu;

  for (int i = 0; i < config.warmup + config.repeat; i++) {
    rtree.clear();
    sw.start();
    rtree.insert(boxes);
    sw.stop();
    ts.insert_ms.push_back(sw.ms());
  }

  for (int i = 0; i < config.warmup + config.repeat; i++) {
    size_t avg_queries =
        (ts.num_queries + config.parallelism - 1) / config.parallelism;
    std::vector<std::thread> threads;

    sw.start();
    ts.num_results = 0;
    results.clear();
    for (int tid = 0; tid < config.parallelism; tid++) {
      threads.emplace_back(
          [&](int tid) {
            auto begin = std::min(tid * avg_queries, ts.num_queries);
            auto end = std::min(begin + avg_queries, ts.num_queries);
            size_t num_results = 0;
            std::vector<box_t> local_results;

            for (auto i = begin; i < end; i++) {
              auto &q = queries[i];
              switch (config.query_type) {
              case BenchmarkConfig::QueryType::kRangeContains:
                rtree.query(boost::geometry::index::contains(q),
                            std::back_inserter(local_results));
                break;
              case BenchmarkConfig::QueryType::kRangeIntersects:
                rtree.query(boost::geometry::index::intersects(q),
                            std::back_inserter(local_results));
                break;
              default:
                abort();
              }
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

  return ts;
}
#endif // SPATIALQUERYBENCHMARK_BOOST_RANGE_QUERY_H
