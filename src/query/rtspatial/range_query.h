#ifndef SPATIALQUERYBENCHMARK_RTSPATIAL_RANGE_QUERY_H
#define SPATIALQUERYBENCHMARK_RTSPATIAL_RANGE_QUERY_H
#include "benchmark_configs.h"
#include "geom_common.h"
#include "time_stat.h"

time_stat RunRangeQueryRTSpatial(const std::vector<box_t> &boxes,
                                 const std::vector<box_t> &queries,
                                 const BenchmarkConfig &config);

time_stat
RunRangeQueryRTSpatialVaryParallelism(const std::vector<box_t> &boxes,
                                      const std::vector<box_t> &queries,
                                      const BenchmarkConfig &config);
#endif // SPATIALQUERYBENCHMARK_RTSPATIAL_RANGE_QUERY_H
