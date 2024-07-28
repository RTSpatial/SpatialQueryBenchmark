
#ifndef SPATIALQUERYBENCHMARK_TIME_STAT_H
#define SPATIALQUERYBENCHMARK_TIME_STAT_H
#include <stdlib.h>

struct time_stat {
  std::vector<double> query_ms;
  std::vector<double> query_ms_after_update;
  std::vector<double> insert_ms;
  std::vector<double> delete_ms;
  std::vector<double> update_ms;
  size_t num_geoms = 0;
  size_t num_queries = 0;
  size_t num_results = 0;
  size_t num_inserts = 0;
  size_t num_deletes = 0;
  size_t num_updates = 0;
};

#endif // SPATIALQUERYBENCHMARK_TIME_STAT_H
