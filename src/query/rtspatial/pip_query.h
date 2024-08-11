#ifndef SPATIALQUERYBENCHMARK_QUERY_RTSPATIAL_LSI_QUERY_H
#define SPATIALQUERYBENCHMARK_QUERY_RTSPATIAL_LSI_QUERY_H
#include "benchmark_configs.h"
#include "geom_common.h"
#include "time_stat.h"

time_stat
RunLSIQueryRTSpatial(const std::shared_ptr<PlanarGraph<double>> &pgraph,
                     const std::shared_ptr<PlanarGraph<double>> &lines2,
                     const BenchmarkConfig &config);
#endif // SPATIALQUERYBENCHMARK_QUERY_RTSPATIAL_LSI_QUERY_H
