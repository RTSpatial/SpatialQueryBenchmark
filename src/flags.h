
#ifndef SPATIALQUERYBENCHMARK_FLAGS_H
#define SPATIALQUERYBENCHMARK_FLAGS_H
#include <gflags/gflags.h>

DECLARE_string(input);
DECLARE_string(output);
DECLARE_string(geom);
DECLARE_string(query);
DECLARE_int32(limit);
DECLARE_int32(min_qualified);
DECLARE_int32(num_queries);
DECLARE_string(query_type);
DECLARE_int32(seed);
DECLARE_string(index_type);
#endif // SPATIALQUERYBENCHMARK_FLAGS_H
