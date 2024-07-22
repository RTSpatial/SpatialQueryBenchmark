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

  boost::geometry::index::rtree<box_t, boost::geometry::index::rstar<16>,
                                boost::geometry::index::indexable<box_t>>
      rtree;
  std::vector<box_t> results;

//  results.reserve(std::max(
//      1ul, size_t(boxes.size() * queries.size() * config.load_factor)));

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

    for (auto &q : queries) {
      switch (config.query_type) {
      case BenchmarkConfig::QueryType::kRangeContains: {
        auto before = results.size();
        rtree.query(boost::geometry::index::contains(q),
                    std::back_inserter(results));
        break;
      }
      case BenchmarkConfig::QueryType::kRangeIntersects: {
        auto before_size = results.size();
        rtree.query(boost::geometry::index::intersects(q),
                    std::back_inserter(results));
        break;
      }
      default:
        abort();
      }
    }
    sw.stop();
    ts.query_ms.push_back(sw.ms());
  }
  ts.num_results = results.size();

  return ts;
}

time_stat RunParallelRangeQueryBoost(const std::vector<box_t> &boxes,
                                     const std::vector<box_t> &queries,
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

//  results.reserve(std::max(
//      1ul, size_t(boxes.size() * queries.size() * config.load_factor)));

  for (int i = 0; i < config.warmup + config.repeat; i++) {
    rtree.clear();
    sw.start();
    rtree.insert(boxes);
    sw.stop();
    ts.insert_ms.push_back(sw.ms());
  }

  for (int i = 0; i < config.warmup + config.repeat; i++) {
    size_t avg_queries = (ts.num_queries + ts.num_threads - 1) / ts.num_threads;
    std::vector<std::thread> threads;
    std::mutex mu;
    results.clear();

    sw.start();
    for (size_t tid = 0; tid < ts.num_threads; tid++) {
      threads.emplace_back(
          [&](size_t tid) {
            auto begin = std::min(tid * avg_queries, ts.num_queries);
            auto end = std::min(begin + avg_queries, ts.num_queries);
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
    ts.num_results = results.size();
  }

  return ts;
}
#endif // SPATIALQUERYBENCHMARK_BOOST_RANGE_QUERY_H
