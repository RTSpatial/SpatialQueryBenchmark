#include <glog/logging.h>

#include <algorithm>
#include <iostream>

#include "benchmark_configs.h"
#include "boost/point_query.h"
#include "boost/range_query.h"
#include "flags.h"
#include "query/glin/point_query.h"
#include "query/glin/range_query.h"
#include "query/rtspatial/point_query.h"
#include "query/rtspatial/range_query.h"

template <typename GEOM_T>
void DumpBoxes(const std::string &output, const std::vector<GEOM_T> &geoms) {
  std::ofstream ofs(output);

  for (auto &geom : geoms) {
    ofs << boost::geometry::to_wkt(geom) << "\n";
  }

  ofs.close();
}

double GetAverageTime(const std::vector<double> &times,
                      const BenchmarkConfig &config) {
  double total_time = 0;

  for (int i = config.warmup; i < times.size(); i++) {
    total_time += times[i];
  }
  return total_time / config.repeat;
}

int main(int argc, char *argv[]) {
  gflags::SetUsageMessage("Usage: ");
  if (argc == 1) {
    gflags::ShowUsageWithFlags(argv[0]);
    exit(1);
  }
  gflags::ParseCommandLineFlags(&argc, &argv, true);

  auto conf = BenchmarkConfig::GetConfig();

  auto geoms = LoadBoxes(conf.geom, conf.serialize, conf.limit);
  std::cout << "Loaded geometries " << geoms.size() << std::endl;
  time_stat ts;

  switch (conf.query_type) {
  case BenchmarkConfig::QueryType::kPointContains: {
    auto queries = LoadPoints(conf.query, conf.limit);

    switch (conf.index_type) {
    case BenchmarkConfig::IndexType::kRTree:
      ts = RunPointQueryBoost(geoms, queries, conf);
      break;
    case BenchmarkConfig::IndexType::kRTreeParallel:
      ts = RunParallelPointQueryBoost(geoms, queries, conf);
      break;
    case BenchmarkConfig::IndexType::kRTSpatial:
      ts = RunPointQueryRTSpatial(geoms, queries, conf);
      break;
    case BenchmarkConfig::IndexType::kGLIN:
      ts = RunPointQueryGLIN(geoms, queries, conf);
      break;
    case BenchmarkConfig::IndexType::kLBVH:
      break;
    }
    break;
  }
  case BenchmarkConfig::QueryType::kRangeContains:
  case BenchmarkConfig::QueryType::kRangeIntersects:
    auto queries = LoadBoxes(conf.query, conf.limit);

    switch (conf.index_type) {
    case BenchmarkConfig::IndexType::kRTree:
      // TODO: passing predicate as a parameter
      ts = RunRangeQueryBoost(geoms, queries, conf);
      break;
    case BenchmarkConfig::IndexType::kRTreeParallel:
      ts = RunParallelRangeQueryBoost(geoms, queries, conf);
      break;
    case BenchmarkConfig::IndexType::kRTSpatial:
      ts = RunRangeQueryRTSpatial(geoms, queries, conf);
      break;
    case BenchmarkConfig::IndexType::kGLIN:
      ts = RunRangeQueryGLIN(geoms, queries, conf);
      break;
    case BenchmarkConfig::IndexType::kLBVH:
      break;
    }
    break;
  }

  if (!ts.insert_ms.empty()) {
    std::cout << "Loading Time " << GetAverageTime(ts.insert_ms, conf) << " ms"
              << std::endl;
  }

  if (!ts.query_ms.empty()) {
    std::cout << "Geoms " << ts.num_geoms << std::endl;
    if (ts.num_threads > 0) {
      std::cout << "Threads " << ts.num_threads << std::endl;
    }
    std::cout << "Queries " << ts.num_queries << std::endl;
    std::cout << "Query Time " << GetAverageTime(ts.query_ms, conf) << " ms"
              << std::endl;
    std::cout << "Results " << ts.num_results << std::endl;
    std::cout << "Selectivity: "
              << (double)ts.num_results / (ts.num_queries * ts.num_geoms)
              << std::endl;
  }

  gflags::ShutDownCommandLineFlags();
}
