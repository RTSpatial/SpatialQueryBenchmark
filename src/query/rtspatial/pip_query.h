#ifndef SPATIALQUERYBENCHMARK_QUERY_RTSPATIAL_LSI_QUERY_H
#define SPATIALQUERYBENCHMARK_QUERY_RTSPATIAL_LSI_QUERY_H
#include "benchmark_configs.h"
#include "geom_common.h"
#include "time_stat.h"

time_stat RunPIPQueryRTSpatial(const std::vector<polygon_t> &polygons,
                               const std::vector<point_t> &points,
                               const BenchmarkConfig &config);
#endif // SPATIALQUERYBENCHMARK_QUERY_RTSPATIAL_LSI_QUERY_H
