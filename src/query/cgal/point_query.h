#ifndef SPATIALQUERYBENCHMARK_CGAL_POINT_QUERY_H
#define SPATIALQUERYBENCHMARK_CGAL_POINT_QUERY_H
#include "stopwatch.h"
#include "time_stat.h"
#include "wkt_loader.h"

#include <CGAL/Fuzzy_iso_box.h>
#include <CGAL/Kd_tree.h>
#include <CGAL/Search_traits_2.h>
#include <CGAL/Simple_cartesian.h>
#include <CGAL/point_generators_2.h>

#include <mutex>
#include <thread>

time_stat RunPointQueryCGAL(const std::vector<box_t> &boxes,
                            const std::vector<point_t> &queries,
                            const BenchmarkConfig &config) {

  typedef CGAL::Simple_cartesian<double> Kernel;
  typedef Kernel::Point_2 Point;
  typedef CGAL::Search_traits_2<Kernel> Traits;
  typedef CGAL::Kd_tree<Traits> Tree;
  typedef CGAL::Fuzzy_iso_box<Traits> Fuzzy_iso_box;

  Stopwatch sw;
  time_stat ts;

  ts.num_geoms = boxes.size();
  ts.num_queries = queries.size();

  Tree tree;
  std::vector<Point> cgal_points;

  for (auto &p : queries) {
    cgal_points.emplace_back(p.x(), p.y());
  }

#ifdef COLLECT_RESULTS
  std::vector<Point> results;
  std::mutex mu;
#endif

#ifdef COUNT_RESULTS
  size_t counter;
#endif

  for (int i = 0; i < config.warmup + config.repeat; i++) {
    tree.clear();
    sw.start();
    tree.insert(cgal_points.begin(), cgal_points.end());
    sw.stop();
    ts.insert_ms.push_back(sw.ms());
  }

  for (int i = 0; i < config.warmup + config.repeat; i++) {
    std::vector<std::thread> threads;
    std::atomic_uint64_t total_num_results;
    size_t avg_queries =
        (boxes.size() + config.parallelism - 1) / config.parallelism;

    sw.start();
    ts.num_results = 0;
#ifdef COLLECT_RESULTS
    results.clear();
#endif
    total_num_results = 0;

    for (int tid = 0; tid < config.parallelism; tid++) {
      threads.emplace_back(
          [&](int tid) {
            auto begin = std::min(tid * avg_queries, boxes.size());
            auto end = std::min(begin + avg_queries, boxes.size());
            size_t num_results = 0;
            std::vector<Point> local_results;

            for (auto i = begin; i < end; i++) {
              auto &p = boxes[i];

              Point lower_left(p.min_corner().x(), p.min_corner().y());
              Point upper_right(p.max_corner().x(), p.max_corner().y());
              Fuzzy_iso_box range(lower_left, upper_right);

#ifdef COLLECT_RESULTS
              tree.search(std::back_inserter(local_results), range);
#endif

#ifdef COUNT_RESULTS
              num_results++;
#endif
            }

#ifdef COLLECT_RESULTS
            std::unique_lock<std::mutex> lock(mu);
            results.insert(results.end(), local_results.begin(),
                           local_results.end());
#endif

#ifdef COUNT_RESULTS
            total_num_results += num_results;
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

#endif // SPATIALQUERYBENCHMARK_CGAL_POINT_QUERY_H
