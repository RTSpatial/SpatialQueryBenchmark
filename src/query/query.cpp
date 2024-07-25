#include <algorithm>
#include <iostream>

#include "benchmark_configs.h"
#include "boost/point_query.h"
#include "boost/range_query.h"

#include "query/glin/range_query.h"
#ifdef USE_GPU
#include "query/lbvh/point_query.h"
#include "query/lbvh/range_query.h"
#include "query/rtspatial/point_query.h"
#include "query/rtspatial/range_query.h"
#endif
#include "flags.h"

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
#ifdef USE_GPU
    case BenchmarkConfig::IndexType::kRTSpatial:
      ts = RunPointQueryRTSpatial(geoms, queries, conf);
      break;
#endif
    case BenchmarkConfig::IndexType::kGLIN:
      //      ts = RunPointQueryGLIN(geoms, queries, conf);
      std::cout << "Unsupported" << std::endl;
      abort();
      break;
#ifdef USE_GPU
    case BenchmarkConfig::IndexType::kLBVH:
      ts = RunPointQueryLBVH(geoms, queries, conf);
      break;
#endif
    default:
      std::cerr << "Invalid Index Type" << std::endl;
      abort();
    }
    break;
  }
  case BenchmarkConfig::QueryType::kRangeContains:
  case BenchmarkConfig::QueryType::kRangeIntersects:
    auto queries = LoadBoxes(conf.query, conf.limit);

    switch (conf.index_type) {
    case BenchmarkConfig::IndexType::kRTree:
      ts = RunRangeQueryBoost(geoms, queries, conf);
      break;
#ifdef USE_GPU
    case BenchmarkConfig::IndexType::kRTSpatial:
      ts = RunRangeQueryRTSpatial(geoms, queries, conf);
      break;
    case BenchmarkConfig::IndexType::kRTSpatialVaryParallelism:
      ts = RunRangeQueryRTSpatialVaryParallelism(geoms, queries, conf);
      break;
#endif
    case BenchmarkConfig::IndexType::kGLIN:
      ts = RunRangeQueryGLIN(geoms, queries, conf);
      break;
#ifdef USE_GPU
    case BenchmarkConfig::IndexType::kLBVH:
      ts = RunRangeQueryLBVH(geoms, queries, conf);
      break;
#endif
    default:
      std::cerr << "Invalid Index Type" << std::endl;
      abort();
    }
    break;
  }

  if (!ts.insert_ms.empty()) {
    std::cout << "Loading Time " << GetAverageTime(ts.insert_ms, conf) << " ms"
              << std::endl;
  }

  if (!ts.query_ms.empty()) {
    std::cout << "Geoms " << ts.num_geoms << std::endl;
    std::cout << "Queries " << ts.num_queries << std::endl;
    if (conf.avg_time) {

      std::cout << "Query Time " << GetAverageTime(ts.query_ms, conf) << " ms"
                << std::endl;
    } else {
      for (size_t i = 0; i < ts.query_ms.size(); i++) {
        std::cout << i << ", Query Time " << ts.query_ms[i] << " ms"
                  << std::endl;
      }
    }
    std::cout << "Results " << ts.num_results << std::endl;
    std::cout << "Selectivity: "
              << (double)ts.num_results / (ts.num_queries * ts.num_geoms)
              << std::endl;
  }

  gflags::ShutDownCommandLineFlags();
}
