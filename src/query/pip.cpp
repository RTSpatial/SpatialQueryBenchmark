#include <iostream>
#include <optix_function_table_definition.h>
#include "benchmark_configs.h"
#include "wkt_loader.h"

#include "flags.h"
#include "query/rtspatial/pip_query.h"

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

  time_stat ts;

  switch (conf.query_type) {
  case BenchmarkConfig::QueryType::kPIP: {
    auto polygons = LoadPolygons(conf.geom, conf.serialize, conf.limit);
    std::cout << "Loaded polygons " << polygons.size() << std::endl;
    auto points = LoadPoints(conf.query, conf.limit);
    std::cout << "Loaded points " << points.size() << std::endl;
    ts = RunPIPQueryRTSpatial(polygons, points, conf);
    break;
  }
  default:
    std::cerr << "Invalid Query Type" << std::endl;
    abort();
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

  if (!ts.query_ms_after_update.empty()) {
    if (conf.avg_time) {
      std::cout << "Query Time After Updates "
                << GetAverageTime(ts.query_ms_after_update, conf) << " ms"
                << std::endl;
    } else {
      for (size_t i = 0; i < ts.query_ms_after_update.size(); i++) {
        std::cout << i << ", Query Time After Updates "
                  << ts.query_ms_after_update[i] << " ms" << std::endl;
      }
    }
  }

  if (ts.num_inserts > 0) {
    std::cout << "Insertion throughput "
              << ts.num_inserts / (GetAverageTime(ts.insert_ms, conf) / 1000.0)
              << " geoms/sec" << std::endl;
  }

  gflags::ShutDownCommandLineFlags();
}
