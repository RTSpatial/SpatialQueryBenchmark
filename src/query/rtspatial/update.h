#ifndef SPATIALQUERYBENCHMARK_RTSPATIAL_UPDATE_H
#define SPATIALQUERYBENCHMARK_RTSPATIAL_UPDATE_H
#include "benchmark_configs.h"
#include "geom_common.h"
#include "time_stat.h"

time_stat RunInsertionRTSpatial(const std::vector<box_t> &boxes,
                                const BenchmarkConfig &config);

time_stat RunDeletionRTSpatial(const std::vector<box_t> &boxes,
                               const BenchmarkConfig &config);


#endif // SPATIALQUERYBENCHMARK_RTSPATIAL_UPDATE_H
