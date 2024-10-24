#ifndef SPATIALQUERYBENCHMARK_GEOM_COMMON_H
#define SPATIALQUERYBENCHMARK_GEOM_COMMON_H
#include "config.h"
#include <boost/geometry.hpp>
using point_t = boost::geometry::model::d2::point_xy<coord_t>;
using line_t = boost::geometry::model::segment<point_t>;
using box_t = boost::geometry::model::box<point_t>;
using polygon_t = boost::geometry::model::polygon<point_t>;
#define BOOST_LEAF_SIZE (256) // suggested by paper
#endif // SPATIALQUERYBENCHMARK_GEOM_COMMON_H
