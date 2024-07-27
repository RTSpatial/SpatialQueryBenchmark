#include "flags.h"
// generator
DEFINE_string(input, "", "path of data file in wkt format");
DEFINE_string(output, "", "path of data file in wkt format");
DEFINE_string(serialize, "", "a directory to store serialized wkt file");
DEFINE_int32(min_qualified, -1, "Intersects per query");
DEFINE_double(
    selectivity, 0.01,
    "Each query guarantees returning selectivity*cardinality geometries");
DEFINE_int32(num_queries, 100, "");
// query
DEFINE_string(geom, "", "path of geom file in wkt format");
DEFINE_string(query, "", "path of query file in wkt format");
DEFINE_double(load_factor, 0.1,
              "Pre-allocated space = factor * |geom| * |query|");
DEFINE_int32(warmup, 5, "Number of warmup rounds");
DEFINE_int32(repeat, 5, "Number of repeated evaluations");
DEFINE_int32(limit, -1, "Read first limit lines");
DEFINE_string(query_type, "", "point-contains/range-contains/range-intersects");
DEFINE_int32(seed, 0, "random seed");
DEFINE_string(index_type, "", "rtree/rtree-parallel/glin/lbvh");
DEFINE_int32(parallelism, -1, "#of cores for CPU baselines");
DEFINE_bool(avg_time, true, "Report average time or list all times");
DEFINE_int32(batch, -1, "Batch size of insertion/deletion");