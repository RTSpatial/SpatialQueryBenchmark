#ifndef SPATIALQUERYBENCHMARK_GENERATOR_H
#define SPATIALQUERYBENCHMARK_GENERATOR_H
#include <boost/geometry/index/rtree.hpp>
#include <boost/iterator/function_output_iterator.hpp>
#include <random>
#include <thread>

#include "geom_common.h"

box_t get_bounds(const std::vector<box_t> &data) {
  auto min_x = std::numeric_limits<coord_t>::max();
  auto min_y = std::numeric_limits<coord_t>::max();
  auto max_x = std::numeric_limits<coord_t>::lowest();
  auto max_y = std::numeric_limits<coord_t>::lowest();

  for (auto &b : data) {
    min_x = std::min(min_x, b.min_corner().x());
    min_y = std::min(min_y, b.min_corner().y());
    max_x = std::max(max_x, b.max_corner().x());
    max_y = std::max(max_y, b.max_corner().y());
  }
  return box_t(point_t(min_x, min_y), point_t(max_x, max_y));
}

/**
 * Generate the queries that there are at least num_qualified input rectangles
 * contain a query
 * @param data
 * @param min_qualified
 * @param num_queries
 * @param seed
 * @return
 */
std::vector<point_t> GeneratePointQueries(const std::vector<box_t> &data,
                                          size_t num_queries, int seed = 0) {
  boost::geometry::index::rtree<box_t, boost::geometry::index::rstar<16>,
                                boost::geometry::index::indexable<box_t>>
      rtree(data);
  auto min_point = rtree.bounds().min_corner();
  auto max_point = rtree.bounds().max_corner();
  std::mt19937 mt(seed);
  std::uniform_real_distribution<double> dist_x(min_point.get<0>(),
                                                max_point.get<0>());
  std::uniform_real_distribution<double> dist_y(min_point.get<1>(),
                                                max_point.get<1>());
  std::vector<point_t> queries;

  for (size_t i = 0; i < num_queries; i++) {
    double x = dist_x(mt);
    double y = dist_y(mt);
    point_t p(x, y);

    rtree.query(boost::geometry::index::nearest(p, 1),
                boost::make_function_output_iterator([&](const box_t &b) {
                  std::uniform_real_distribution<double> dist_box_x(
                      b.min_corner().x(), b.max_corner().x());
                  std::uniform_real_distribution<double> dist_box_y(
                      b.min_corner().y(), b.max_corner().y());

                  point_t point_in_box(dist_box_x(mt), dist_box_y(mt));
                  queries.push_back(point_in_box);
                }));
  }

  return queries;
}

/**
 * Generate the queries that there are at least num_qualified input rectangles
 * contain a query
 * @param data
 * @param min_qualified
 * @param num_queries
 * @param seed
 * @return
 */
std::vector<box_t> GenerateContainsQueries(const std::vector<box_t> &data,
                                           size_t num_queries, int seed = 0) {
  boost::geometry::index::rtree<box_t, boost::geometry::index::rstar<16>,
                                boost::geometry::index::indexable<box_t>>
      rtree(data);
  auto min_point = rtree.bounds().min_corner();
  auto max_point = rtree.bounds().max_corner();
  std::mt19937 mt(seed);
  std::uniform_real_distribution<double> dist_x(min_point.get<0>(),
                                                max_point.get<0>());
  std::uniform_real_distribution<double> dist_y(min_point.get<1>(),
                                                max_point.get<1>());
  std::vector<box_t> queries;

  for (size_t i = 0; i < num_queries; i++) {
    double x = dist_x(mt);
    double y = dist_y(mt);
    point_t p(x, y);

    rtree.query(boost::geometry::index::nearest(p, 1),
                boost::make_function_output_iterator([&](const box_t &b) {
                  auto width = b.max_corner().x() - b.min_corner().x();
                  auto height = b.max_corner().y() - b.min_corner().y();

                  std::uniform_real_distribution<double> dist_min_x(
                      b.min_corner().x(), b.max_corner().x());
                  std::uniform_real_distribution<double> dist_min_y(
                      b.min_corner().y(), b.max_corner().y());
                  auto min_x = dist_min_x(mt);
                  auto min_y = dist_min_y(mt);
                  std::uniform_real_distribution<double> dist_max_x(
                      min_x, b.max_corner().x());
                  std::uniform_real_distribution<double> dist_max_y(
                      min_y, b.max_corner().y());
                  auto max_x = dist_max_x(mt);
                  auto max_y = dist_max_y(mt);
                  box_t query(point_t(min_x, min_y), point_t(max_x, max_y));

                  queries.push_back(query);
                }));
  }

  return queries;
}

/**
 * Generate the queries that each query at least intersects at least
 * min_qualified rectangles
 * @param data
 * @param min_qualified
 * @param num_queries
 * @param seed
 * @return
 */
std::vector<box_t> GenerateIntersectsQueries(const std::vector<box_t> &data,
                                             size_t min_qualified,
                                             size_t num_queries, int seed = 0) {
  boost::geometry::index::rtree<box_t,
                                boost::geometry::index::linear<BOOST_LEAF_SIZE>,
                                boost::geometry::index::indexable<box_t>>
      rtree(data);
  auto bounds = get_bounds(data);

  printf("range x [%f, %f], y [%f, %f]\n", bounds.min_corner().x(),
         bounds.max_corner().x(), bounds.min_corner().y(),
         bounds.max_corner().y());

  std::mt19937 mt(seed);
  std::vector<box_t> queries;
  std::atomic_uint64_t total_intersects = 0;

  auto n_threads = std::thread::hardware_concurrency();
  auto avg_queries = (num_queries + n_threads - 1) / n_threads;
  std::vector<std::thread> threads;
  std::mutex mu;

  for (uint32_t tid = 0; tid < n_threads; tid++) {
    threads.emplace_back(
        [&](int tid) {
          std::uniform_real_distribution<coord_t> dist_x(
              bounds.min_corner().x(), bounds.max_corner().x());
          std::uniform_real_distribution<coord_t> dist_y(
              bounds.min_corner().y(), bounds.max_corner().y());
          auto begin = std::min(tid * avg_queries, num_queries);
          auto end = std::min(begin + avg_queries, num_queries);
          std::vector<box_t> local_queries;

          for (auto i = begin; i < end; i++) {
          retry:
            auto x = dist_x(mt);
            auto y = dist_y(mt);
            point_t p(x, y);
            auto min_x = x;
            auto min_y = y;
            auto max_x = x;
            auto max_y = y;

            std::vector<box_t> results;

            rtree.query(boost::geometry::index::nearest(p, min_qualified),
                        std::back_inserter(results));
            box_t query;

            for (const box_t &nbr_box : results) {
              min_x = std::min(min_x, nbr_box.min_corner().x());
              min_y = std::min(min_y, nbr_box.min_corner().y());
              max_x = std::max(max_x, nbr_box.max_corner().x());
              max_y = std::max(max_y, nbr_box.max_corner().y());
              query = box_t(point_t(min_x, min_y), point_t(max_x, max_y));
              uint32_t n_intersects = 0;

              rtree.query(boost::geometry::index::intersects(query),
                          boost::make_function_output_iterator(
                              [&](const box_t &b) { n_intersects++; }));
              if (n_intersects >= min_qualified) {
                // too many intersections, retry to generate another query
                if (min_qualified > 1 &&
                    (float)n_intersects / min_qualified > 10) {
                  goto retry;
                }
                total_intersects += n_intersects;
                break;
              }
            }

            local_queries.push_back(query);
          }

          std::unique_lock<std::mutex> lock(mu);
          queries.insert(queries.end(), local_queries.begin(),
                         local_queries.end());
        },
        tid);
  }
  for (auto &thread : threads) {
    thread.join();
  }

  std::cout << "Real selectivity "
            << (float)total_intersects / (data.size() * num_queries)
            << std::endl;

  return queries;
}

std::vector<box_t> GenerateUniformQueries(const std::vector<box_t> &data,
                                          float fraction, size_t num_queries,
                                          int seed = 0) {
  boost::geometry::index::rtree<box_t, boost::geometry::index::rstar<16>,
                                boost::geometry::index::indexable<box_t>>
      rtree(data);
  auto min_point = rtree.bounds().min_corner();
  auto max_point = rtree.bounds().max_corner();
  std::mt19937 mt(seed);
  std::uniform_real_distribution<coord_t> dist_x(min_point.get<0>(),
                                                 max_point.get<0>());
  std::uniform_real_distribution<coord_t> dist_y(min_point.get<1>(),
                                                 max_point.get<1>());
  std::vector<box_t> queries;

  coord_t width = max_point.get<0>() - min_point.get<0>();
  coord_t height = max_point.get<1>() - min_point.get<1>();

  coord_t width_frac = width * sqrt(fraction);
  coord_t height_frac = height * sqrt(fraction);

  for (size_t i = 0; i < num_queries; i++) {
    auto x = dist_x(mt);
    auto y = dist_y(mt);
    point_t p(x, y);
    auto min_x = x;
    auto min_y = y;

    float max_x;
    float max_y;

    do {
      max_x = x + width_frac;
      max_y = y + height_frac;
      x = dist_x(mt);
      y = dist_y(mt);
    } while (max_x > max_point.get<0>() || max_y > max_point.get<1>());

    box_t query(point_t(min_x, min_y), point_t(max_x, max_y));

    //    size_t result_size = 0;
    //
    //    rtree.query(boost::geometry::index::intersects(query),
    //                boost::make_function_output_iterator(
    //                    [&](const box_t &b) { result_size++; }));

    //    if(result_size > 1000) {
    //      std::cout << "intersects " << result_size << std::endl;
    //    }

    //    std::cout << "Result frac " << (float)result_size / data.size()
    //              << std::endl;

    queries.push_back(query);
  }

  return queries;
}

#endif // SPATIALQUERYBENCHMARK_GENERATOR_H
