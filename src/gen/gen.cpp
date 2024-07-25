#include <algorithm>
#include <iostream>

#include "flags.h"
#include "generator.h"
#include "wkt_loader.h"

template <typename GEOM_T>
void DumpBoxes(const std::string &output, const std::vector<GEOM_T> &geoms) {
  std::ofstream ofs(output);

  for (auto &geom : geoms) {
    ofs << boost::geometry::to_wkt(geom, 14) << "\n";
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

  std::string input = FLAGS_input;
  std::string output = FLAGS_output;
  std::string query_type = FLAGS_query_type;
  int limit = FLAGS_limit;
  int min_qualified = FLAGS_min_qualified;
  float selectivity = FLAGS_selectivity;
  int num_queries = FLAGS_num_queries;
  int seed = FLAGS_seed;

  if (limit == -1) {
    limit = std::numeric_limits<int>::max();
  }

  if (access(input.c_str(), R_OK) != 0) {
    std::cerr << "Cannot open " << input << std::endl;
    abort();
  }

  auto geoms = LoadBoxes(FLAGS_input, limit);
  std::cout << "Loaded geometries " << geoms.size() << std::endl;

  if (min_qualified == -1) {
    min_qualified = std::max(1ul, (size_t)(geoms.size() * selectivity));
    std::cout << "Selectivity " << selectivity << ", Min qualified per query "
              << min_qualified << std::endl;
  }

  if (query_type == "point-contains") {
    auto queries = GeneratePointQueries(geoms, num_queries, seed);

    DumpBoxes(output, queries);
  } else if (query_type == "range-contains") {
    auto queries = GenerateContainsQueries(geoms, num_queries, seed);

    DumpBoxes(output, queries);
  } else if (query_type == "range-intersects") {
    auto queries =
        GenerateIntersectsQueries(geoms, min_qualified, num_queries, seed);

    //    auto queries = GenerateUniformQueries(geoms, 0.001, num_queries,
    //    seed);
    DumpBoxes(output, queries);
  } else {
    std::cerr << "Not supported query type: " << FLAGS_query_type << std::endl;
    abort();
  }

  gflags::ShutDownCommandLineFlags();
}
