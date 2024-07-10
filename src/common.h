#ifndef SPATIALQUERYBENCHMARK_COMMON_H
#define SPATIALQUERYBENCHMARK_COMMON_H
#include <boost/geometry.hpp>

using point_t = boost::geometry::model::d2::point_xy<double>;
using box_t = boost::geometry::model::box<point_t>;
using polygon_t = boost::geometry::model::polygon<point_t>;

#endif // SPATIALQUERYBENCHMARK_COMMON_H
