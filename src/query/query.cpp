#include <glog/logging.h>

#include <algorithm>
#include <iostream>

#include "benchmark_configs.h"
#include "flags.h"
#include "loader.h"
#include "point_query.h"
#include "range_query.h"

template <typename GEOM_T>
void DumpBoxes(const std::string &output, const std::vector<GEOM_T> &geoms) {
  std::ofstream ofs(output);

  for (auto &geom : geoms) {
    ofs << boost::geometry::to_wkt(geom) << "\n";
  }

  ofs.close();
}

int main(int argc, char *argv[]) {
  gflags::SetUsageMessage("Usage: ");
  if (argc == 1) {
    gflags::ShowUsageWithFlags(argv[0]);
    exit(1);
  }
  gflags::ParseCommandLineFlags(&argc, &argv, true);

  auto conf = BenchmarkConfig::GetConfig();

  auto geoms = LoadBoxes(conf.geom, conf.limit);
  std::cout << "Loaded geometries " << geoms.size() << std::endl;
  time_stat ts;

  std::cout << "query type " << (int) conf.query_type << std::endl;

  switch (conf.query_type) {
  case BenchmarkConfig::QueryType::kPointContains: {
    auto queries = LoadPoints(conf.query, conf.limit);

    switch (conf.index_type) {
    case BenchmarkConfig::IndexType::kRTree:
      ts = RunPointQuery(geoms, queries);
      break;
    case BenchmarkConfig::IndexType::kRTreeParallel:
      ts = RunParallelPointQuery(geoms, queries);
      break;
    case BenchmarkConfig::IndexType::kGLIN:
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
      ts = RunRangeQuery(geoms, queries, conf.query_type);
      break;
    case BenchmarkConfig::IndexType::kRTreeParallel:
      ts = RunParallelRangeQuery(geoms, queries, conf.query_type);
      break;
    case BenchmarkConfig::IndexType::kGLIN:
      break;
    case BenchmarkConfig::IndexType::kLBVH:
      break;
    }
    break;
  }

  if (ts.load_ms > 0) {
    std::cout << "Loading Time " << ts.load_ms << " ms" << std::endl;
  }

  if (ts.query_ms > 0) {
    std::cout << "Geoms " << ts.num_geoms << std::endl;
    std::cout << "Queries " << ts.num_queries << std::endl;
    std::cout << "Query Time " << ts.query_ms << " ms" << std::endl;
    std::cout << "Results " << ts.num_results << std::endl;
  }

  gflags::ShutDownCommandLineFlags();
}
