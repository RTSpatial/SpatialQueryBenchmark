#ifndef SPATIALQUERYBENCHMARK_GEOM_COMMON_H
#define SPATIALQUERYBENCHMARK_GEOM_COMMON_H
#include <boost/geometry.hpp>
using coord_t = float;
using point_t = boost::geometry::model::d2::point_xy<coord_t>;
using box_t = boost::geometry::model::box<point_t>;
using polygon_t = boost::geometry::model::polygon<point_t>;

#endif // SPATIALQUERYBENCHMARK_GEOM_COMMON_H
