#ifndef SPATIALQUERYBENCHMARK_RT_SPATIAL_POINT_QUERY_H
#define SPATIALQUERYBENCHMARK_RT_SPATIAL_POINT_QUERY_H
#include "benchmark_configs.h"
#include "geom_common.h"
#include "time_stat.h"

time_stat RunPointQueryRTSpatial(const std::vector<box_t> &boxes,
                                 const std::vector<point_t> &queries,
                                 const BenchmarkConfig &config);
#endif // SPATIALQUERYBENCHMARK_RT_SPATIAL_POINT_QUERY_H
