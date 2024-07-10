
#ifndef SPATIALQUERYBENCHMARK_TIME_STAT_H
#define SPATIALQUERYBENCHMARK_TIME_STAT_H
#include <stdlib.h>

struct time_stat {
  double load_ms = 0;
  double query_ms = 0;
  double insert_ms = 0;
  double delete_ms = 0;
  double update_ms = 0;
  size_t num_geoms = 0;
  size_t num_queries = 0;
  size_t num_results = 0;
  size_t num_inserts = 0;
  size_t num_deletes = 0;
  size_t num_updates = 0;
  size_t num_threads = 0;
};

#endif // SPATIALQUERYBENCHMARK_TIME_STAT_H
