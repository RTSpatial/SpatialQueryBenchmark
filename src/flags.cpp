#include "flags.h"
// generator
DEFINE_string(input, "", "path of data file in wkt format");
DEFINE_string(output, "", "path of data file in wkt format");
DEFINE_int32(min_qualified, 10, "Intersects per query");
DEFINE_int32(max_point_in_box, 10, "Points per query");
DEFINE_int32(num_queries, 100, "");
// query
DEFINE_string(geom, "", "path of geom file in wkt format");
DEFINE_string(query, "", "path of query file in wkt format");
DEFINE_int32(limit, -1, "Read first limit lines");
DEFINE_string(query_type, "", "point-contains/range-contains/range-intersects");
DEFINE_int32(seed, 0, "random seed");
DEFINE_string(index_type, "", "rtree/rtree-parallel/glin/lbvh");