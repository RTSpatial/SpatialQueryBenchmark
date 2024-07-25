#ifndef SPATIALQUERYBENCHMARK_LBVH_RANGE_QUERY_H
#define SPATIALQUERYBENCHMARK_LBVH_RANGE_QUERY_H
#include "benchmark_configs.h"
#include "geom_common.h"
#include "time_stat.h"

time_stat RunRangeQueryLBVH(const std::vector<box_t> &boxes,
                            const std::vector<box_t> &queries,
                            const BenchmarkConfig &config);

#endif // SPATIALQUERYBENCHMARK_LBVH_RANGE_QUERY_H
