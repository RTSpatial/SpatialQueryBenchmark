
#ifndef SPATIALQUERYBENCHMARK_CONFIGS_H
#define SPATIALQUERYBENCHMARK_CONFIGS_H
#include "flags.h"
#include <iostream>
#include <limits>
#include <string>
#include <thread>
#include <unistd.h>

struct BenchmarkConfig {
  enum class QueryType {
    kPointContains,
    kRangeContains,
    kRangeIntersects,
    kBulkLoading,
    kInsertion,
    kDeletion
  };

  enum class IndexType {
    kCGAL,
    kGLIN,
    kLBVH,
    kRTree,
    kRTSpatial,
    kRTSpatialVaryParallelism
  };

  std::string geom;
  std::string query;
  std::string serialize;
  int warmup;
  int repeat;
  int limit;
  int seed;
  int parallelism;
  bool avg_time;
  QueryType query_type;
  IndexType index_type;
  float load_factor;
  int batch;

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
    config.parallelism = FLAGS_parallelism;
    config.avg_time = FLAGS_avg_time;
    config.batch = FLAGS_batch;

    if (config.limit == -1) {
      config.limit = std::numeric_limits<int>::max();
    }

    if (config.parallelism == -1) {
      config.parallelism = std::thread::hardware_concurrency();
    }

    if (access(config.geom.c_str(), R_OK) != 0) {
      std::cerr << "Cannot open " << config.geom << std::endl;
      abort();
    }

    if (!config.query.empty() && access(config.query.c_str(), R_OK) != 0) {
      std::cerr << "Cannot open " << config.query << std::endl;
      abort();
    }

    if (FLAGS_query_type == "point-contains") {
      config.query_type = BenchmarkConfig::QueryType::kPointContains;
    } else if (FLAGS_query_type == "range-contains") {
      config.query_type = BenchmarkConfig::QueryType::kRangeContains;
    } else if (FLAGS_query_type == "range-intersects") {
      config.query_type = BenchmarkConfig::QueryType::kRangeIntersects;
    } else if (FLAGS_query_type == "bulk-loading") {
      config.query_type = BenchmarkConfig::QueryType::kBulkLoading;
    } else if (FLAGS_query_type == "insertion") {
      config.query_type = BenchmarkConfig::QueryType::kInsertion;
    } else if (FLAGS_query_type == "deletion") {
      config.query_type = BenchmarkConfig::QueryType::kDeletion;
    } else {
      std::cerr << "Invalid query " << FLAGS_query << std::endl;
      abort();
    }

    if (FLAGS_index_type == "cgal") {
      config.index_type = IndexType::kCGAL;
    } else if (FLAGS_index_type == "rtree") {
      config.index_type = IndexType::kRTree;
    } else if (FLAGS_index_type == "rtspatial") {
      config.index_type = IndexType::kRTSpatial;
    } else if (FLAGS_index_type == "rtspatial-vary-parallelism") {
      config.index_type = IndexType::kRTSpatialVaryParallelism;
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
