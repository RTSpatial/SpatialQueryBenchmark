
#ifndef SPATIALQUERYBENCHMARK_CONFIGS_H
#define SPATIALQUERYBENCHMARK_CONFIGS_H
#include "flags.h"
#include <iostream>
#include <limits>
#include <string>
#include <unistd.h>

struct BenchmarkConfig {
  enum class QueryType { kPointContains, kRangeContains, kRangeIntersects };

  enum class IndexType { kRTree, kRTreeParallel, kRTSpatial, kGLIN, kLBVH };

  std::string geom;
  std::string query;
  std::string serialize;
  int warmup;
  int repeat;
  int limit;
  int seed;
  QueryType query_type;
  IndexType index_type;
  float load_factor;

  static BenchmarkConfig GetConfig() {
    BenchmarkConfig config;

    config.geom = FLAGS_geom;
    config.query = FLAGS_query;
    config.serialize = FLAGS_serialize;
    config.warmup = FLAGS_warmup;
    config.repeat = FLAGS_repeat;
    config.load_factor = FLAGS_load_factor;
    config.limit = FLAGS_limit;
    config.seed = FLAGS_seed;
    config.repeat = FLAGS_repeat;

    if (config.limit == -1) {
      config.limit = std::numeric_limits<int>::max();
    }

    if (access(config.geom.c_str(), R_OK) != 0) {
      std::cerr << "Cannot open " << config.geom << std::endl;
      abort();
    }

    if (access(config.query.c_str(), R_OK) != 0) {
      std::cerr << "Cannot open " << config.query << std::endl;
      abort();
    }

    if (FLAGS_query_type == "point-contains") {
      config.query_type = BenchmarkConfig::QueryType::kPointContains;
    } else if (FLAGS_query_type == "range-contains") {
      config.query_type = BenchmarkConfig::QueryType::kRangeContains;
    } else if (FLAGS_query_type == "range-intersects") {
      config.query_type = BenchmarkConfig::QueryType::kRangeIntersects;
    } else {
      std::cerr << "Invalid query " << FLAGS_query << std::endl;
      abort();
    }

    if (FLAGS_index_type == "rtree") {
      config.index_type = IndexType::kRTree;
    } else if (FLAGS_index_type == "rtree-parallel") {
      config.index_type = IndexType::kRTreeParallel;
    } else if (FLAGS_index_type == "rtspatial") {
      config.index_type = IndexType::kRTSpatial;
    } else if (FLAGS_index_type == "glin") {
      config.index_type = IndexType::kGLIN;
    } else if (FLAGS_index_type == "lbvh") {
      config.index_type = IndexType::kLBVH;
    } else {
      std::cerr << "Invalid index type " << FLAGS_index_type << std::endl;
      abort();
    }

    return config;
  }
};

#endif // SPATIALQUERYBENCHMARK_CONFIGS_H
