#ifndef SPATIALQUERYBENCHMARK_GENERATOR_H
#define SPATIALQUERYBENCHMARK_GENERATOR_H
#include <random>

#include <boost/geometry/index/rtree.hpp>
#include <boost/iterator/function_output_iterator.hpp>

#include "geom_common.h"

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

  for (size_t i = 0; i < num_queries; i++) {
    auto x = dist_x(mt);
    auto y = dist_y(mt);
    point_t p(x, y);
    auto min_x = x;
    auto min_y = y;
    auto max_x = x;
    auto max_y = y;

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
