#ifndef SPATIALQUERYBENCHMARK_GENERATOR_H
#define SPATIALQUERYBENCHMARK_GENERATOR_H
#include <random>

#include <boost/geometry/index/rtree.hpp>
#include <boost/iterator/function_output_iterator.hpp>

#include "common.h"

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
                                          size_t max_points_per_box,
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
  std::uniform_int_distribution<size_t> dist_num_points(1, max_points_per_box);
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
                  size_t n_points_in_box = dist_num_points(mt);

                  for (int j = 0; j < n_points_in_box; j++) {
                    point_t point_in_box(dist_box_x(mt), dist_box_y(mt));

                    queries.push_back(point_in_box);
                  }
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
                  auto min_x = b.min_corner().x() + width / 4;
                  auto max_x = b.max_corner().x() - width / 4;
                  auto min_y = b.min_corner().y() + height / 4;
                  auto max_y = b.max_corner().y() - height / 4;
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
    double min_x = x;
    double min_y = y;
    double max_x = x;
    double max_y = y;

    rtree.query(boost::geometry::index::nearest(p, min_qualified),
                boost::make_function_output_iterator([&](const box_t &b) {
                  min_x = std::min(min_x, b.min_corner().x());
                  min_y = std::min(min_y, b.min_corner().y());
                  max_x = std::max(max_x, b.max_corner().x());
                  max_y = std::max(max_y, b.max_corner().y());
                }));

    box_t query(point_t(min_x, min_y), point_t(max_x, max_y));
    queries.push_back(query);
  }

  return queries;
}

#endif // SPATIALQUERYBENCHMARK_GENERATOR_H
