
#ifndef SPATIALQUERYBENCHMARK_RANGE_QUERY_H
#define SPATIALQUERYBENCHMARK_RANGE_QUERY_H
#include "loader.h"
#include "stopwatch.h"
#include "time_stat.h"
#include <mutex>
#include <thread>

time_stat RunRangeQuery(const std::vector<box_t> &boxes,
                        const std::vector<box_t> &queries,
                        BenchmarkConfig::QueryType query_type) {
  Stopwatch sw;
  time_stat ts;

  ts.num_geoms = boxes.size();
  ts.num_queries = queries.size();

  sw.start();
  boost::geometry::index::rtree<box_t, boost::geometry::index::rstar<16>,
                                boost::geometry::index::indexable<box_t>>
      rtree(boxes);
  sw.stop();
  ts.load_ms = sw.ms();
  std::vector<box_t> results;

  sw.start();
  for (auto &q : queries) {
    switch (query_type) {
    case BenchmarkConfig::QueryType::kRangeContains:
      rtree.query(boost::geometry::index::contains(q),
                  std::back_inserter(results));
      break;
    case BenchmarkConfig::QueryType::kRangeIntersects:
      rtree.query(boost::geometry::index::intersects(q),
                  std::back_inserter(results));
      break;
    default:
      std::cout << "Error" << std::endl;
    }
  }
  sw.stop();
  ts.query_ms = sw.ms();
  ts.num_results = results.size();

  return ts;
}

time_stat RunParallelRangeQuery(const std::vector<box_t> &boxes,
                                const std::vector<box_t> &queries,
                                BenchmarkConfig::QueryType query_type) {
  Stopwatch sw;
  time_stat ts;

  ts.num_geoms = boxes.size();
  ts.num_queries = queries.size();
  ts.num_threads = std::thread::hardware_concurrency();

  sw.start();
  boost::geometry::index::rtree<box_t, boost::geometry::index::rstar<16>,
                                boost::geometry::index::indexable<box_t>>
      rtree(boxes);
  sw.stop();
  ts.load_ms = sw.ms();
  std::vector<box_t> results;
  std::vector<std::thread> threads;
  std::mutex mu;

  sw.start();
  size_t avg_queries = (ts.num_queries + ts.num_threads - 1) / ts.num_threads;
  for (size_t tid = 0; tid < ts.num_threads; tid++) {
    threads.emplace_back(std::thread(
        [&](size_t tid) {
          auto begin = std::min(tid * avg_queries, ts.num_queries);
          auto end = std::min(begin + avg_queries, ts.num_queries);
          std::vector<box_t> local_results;

          for (auto i = begin; i < end; i++) {
            auto &q = queries[i];
            switch (query_type) {
            case BenchmarkConfig::QueryType::kRangeContains:
              rtree.query(boost::geometry::index::contains(q),
                          std::back_inserter(results));
              break;
            case BenchmarkConfig::QueryType::kRangeIntersects:
              rtree.query(boost::geometry::index::intersects(q),
                          std::back_inserter(results));
              break;
            }
          }
          {
            std::unique_lock<std::mutex> lock(mu);
            results.insert(results.end(), local_results.begin(),
                           local_results.end());
          }
        },
        tid));
  }
  for (auto &thread : threads) {
    thread.join();
  }
  sw.stop();
  ts.query_ms = sw.ms();
  ts.num_results = results.size();

  return ts;
}
#endif // SPATIALQUERYBENCHMARK_RANGE_QUERY_H
