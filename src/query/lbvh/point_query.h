#ifndef SPATIALQUERYBENCHMARK_LBVH_POINT_QUERY_CUH
#define SPATIALQUERYBENCHMARK_LBVH_POINT_QUERY_CUH
#include "benchmark_configs.h"
#include "geom_common.h"
#include "time_stat.h"

time_stat RunPointQueryLBVH(const std::vector<box_t> &boxes,
                            const std::vector<point_t> &queries,
                            const BenchmarkConfig &config);
#endif // SPATIALQUERYBENCHMARK_LBVH_POINT_QUERY_CUH
