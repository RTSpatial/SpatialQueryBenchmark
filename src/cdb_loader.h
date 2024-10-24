#ifndef SPATIALQUERYBENCHMARK_CBD_LOADER_H
#define SPATIALQUERYBENCHMARK_CBD_LOADER_H
#include <cuda_runtime.h>
#include <dirent.h>
#include <sys/stat.h>

#include <algorithm>
#include <fstream>
#include <map>
#include <random>
#include <vector>

#include "geom_common.h"

#define STREAM_WRITE_VAR(stream, var)                                          \
  stream.write(reinterpret_cast<const char *>(&(var)), sizeof(var));
#define STREAM_READ_VAR(stream, var)                                           \
  stream.read(reinterpret_cast<char *>(&var), sizeof(var));

struct Chain {
  int64_t id;              // chain index
  int64_t first_point_idx; // unused, first, last index of the chain
  int64_t last_point_idx;
  int64_t left_polygon_id;  // left polygon id of the chain
  int64_t right_polygon_id; // right polygon id of the chain
};

template <typename COORD_T> struct PlanarGraph {
  using point_t = boost::geometry::model::d2::point_xy<COORD_T>;
  std::vector<Chain> chains;
  std::vector<uint32_t> row_index; // organized in chains
  std::vector<point_t> points;
  boost::geometry::model::box<point_t> bb;
};

template <typename COORD_T>
inline std::shared_ptr<PlanarGraph<COORD_T>> read_pgraph(const char *path) {
  std::ifstream ifs(path);

  if (!ifs.is_open()) {
    std::cerr << "Cannot open file " << path;
    abort();
  }

  std::string line;
  Chain *curr_chain;
  int64_t np = 0;
  auto pgraph = std::make_shared<PlanarGraph<COORD_T>>();
  auto &g = *pgraph;
  typename PlanarGraph<COORD_T>::point_t *last_p = nullptr;
  std::vector<double> seg_lens;
  size_t lno = 0;

  while (std::getline(ifs, line)) {
    lno++;
    if (line.empty() || line[0] == '#' || line[0] == '%') {
      continue;
    }
    std::istringstream iss(line);
    bool bad_line;

    if (np == 0) {
      g.chains.push_back(Chain());
      curr_chain = &g.chains.back();

      bad_line = !(iss >> curr_chain->id >> np >> curr_chain->first_point_idx >>
                   curr_chain->last_point_idx >> curr_chain->left_polygon_id >>
                   curr_chain->right_polygon_id);
      bad_line |= np < 2;
      // checking overlapped polygon
      //      bad_line |= curr_chain->left_polygon_id ==
      //      curr_chain->right_polygon_id;
      pgraph->row_index.push_back(g.points.size());
      last_p = nullptr;
    } else {
      COORD_T x, y;

      bad_line = !(iss >> x >> y);

      if (last_p != nullptr) {
        auto seg_len = sqrt((x - last_p->x()) * (x - last_p->x()) +
                            (y - last_p->y()) * (y - last_p->y()));
        seg_lens.push_back(seg_len);
        bad_line |= x == last_p->x() && y == last_p->y();
      }

      g.bb.min_corner().x(std::min(g.bb.min_corner().x(), x));
      g.bb.max_corner().x(std::max(g.bb.max_corner().x(), x));
      g.bb.min_corner().y(std::min(g.bb.min_corner().y(), y));
      g.bb.max_corner().y(std::max(g.bb.max_corner().y(), y));

      typename PlanarGraph<COORD_T>::point_t p(x, y);
      g.points.push_back(p);
      last_p = &g.points.back();
      np--;
    }

    if (bad_line) {
      std::cerr << "Bad line. Check your dataset! " << path << "[" << lno
                << "]: " << line;
      abort();
    }
  }
  ifs.close();

  if (!g.points.empty()) { // in case of an empty graph
    pgraph->row_index.push_back(g.points.size());
  }
  if (np != 0) {
    std::cerr << "Bad file " << path << std::endl;
    abort();
  }

  double total_seg_len = std::accumulate(seg_lens.begin(), seg_lens.end(), 0.0);
  double mean = total_seg_len / seg_lens.size();

  std::vector<double> diff(seg_lens.size());
  std::transform(seg_lens.begin(), seg_lens.end(), diff.begin(),
                 [mean](double x) { return x - mean; });
  double sq_sum =
      std::inner_product(diff.begin(), diff.end(), diff.begin(), 0.0);
  double stdev = std::sqrt(sq_sum / seg_lens.size());

  std::cout << "Map " << path << " is loaded, chains: " << g.chains.size()
            << " points: " << pgraph->points.size()
            << " edges: " << g.points.size() - g.chains.size()
            << ", min seg len: "
            << *std::min_element(seg_lens.begin(), seg_lens.end())
            << ", max seg len: "
            << *std::max_element(seg_lens.begin(), seg_lens.end())
            << ", avg seg len: " << mean << ", stdev: " << stdev << std::endl;
  return pgraph;
}

template <typename COORD_T>
inline void serialize_pgraph(std::shared_ptr<PlanarGraph<COORD_T>> pgraph,
                             const char *path) {
  std::ofstream ofs;
  ofs.open(path, std::ios::out | std::ios::binary);

  uint64_t n_chains = pgraph->chains.size();
  uint64_t n_row_index = pgraph->row_index.size();
  uint64_t n_points = pgraph->points.size();
  uint64_t check_sum = 0xabcdabcd;

  assert(ofs.good());

  STREAM_WRITE_VAR(ofs, check_sum);
  STREAM_WRITE_VAR(ofs, n_chains);
  STREAM_WRITE_VAR(ofs, n_row_index);
  STREAM_WRITE_VAR(ofs, n_points);

  for (auto &chain : pgraph->chains) {
    STREAM_WRITE_VAR(ofs, chain.id);
    STREAM_WRITE_VAR(ofs, chain.first_point_idx);
    STREAM_WRITE_VAR(ofs, chain.last_point_idx);
    STREAM_WRITE_VAR(ofs, chain.left_polygon_id);
    STREAM_WRITE_VAR(ofs, chain.right_polygon_id);
  }
  for (auto &idx : pgraph->row_index) {
    STREAM_WRITE_VAR(ofs, idx);
  }
  for (auto &p : pgraph->points) {
    STREAM_WRITE_VAR(ofs, p.x());
    STREAM_WRITE_VAR(ofs, p.y());
  }
  STREAM_WRITE_VAR(ofs, pgraph->bb.min_corner().x());
  STREAM_WRITE_VAR(ofs, pgraph->bb.min_corner().y());
  STREAM_WRITE_VAR(ofs, pgraph->bb.max_corner().x());
  STREAM_WRITE_VAR(ofs, pgraph->bb.max_corner().y());
  STREAM_WRITE_VAR(ofs, check_sum);

  ofs.close();
}

template <typename COORD_T>
inline std::shared_ptr<PlanarGraph<COORD_T>>
deserialize_pgraph(const char *path) {
  std::ifstream ifs;
  ifs.open(path, std::ios::in | std::ios::binary);
  auto pgraph = std::make_shared<PlanarGraph<COORD_T>>();

  uint64_t check_sum;
  uint64_t n_chains;
  uint64_t n_row_index;
  uint64_t n_points;

  assert(ifs.good());

  STREAM_READ_VAR(ifs, check_sum);
  if (check_sum != 0xabcdabcd) {
    std::cerr << "Bad checksum " << path << std::endl;
    abort();
  }

  STREAM_READ_VAR(ifs, n_chains);
  STREAM_READ_VAR(ifs, n_row_index);
  STREAM_READ_VAR(ifs, n_points);
  pgraph->chains.resize(n_chains);
  pgraph->row_index.resize(n_row_index);
  pgraph->points.resize(n_points);

  for (auto &chain : pgraph->chains) {
    STREAM_READ_VAR(ifs, chain.id);
    STREAM_READ_VAR(ifs, chain.first_point_idx);
    STREAM_READ_VAR(ifs, chain.last_point_idx);
    STREAM_READ_VAR(ifs, chain.left_polygon_id);
    STREAM_READ_VAR(ifs, chain.right_polygon_id);
  }
  for (auto &idx : pgraph->row_index) {
    STREAM_READ_VAR(ifs, idx);
  }
  for (auto &p : pgraph->points) {
    COORD_T x, y;
    STREAM_READ_VAR(ifs, x);
    STREAM_READ_VAR(ifs, y);
    p = typename PlanarGraph<COORD_T>::point_t(x, y);
  }

  COORD_T min_x, min_y, max_x, max_y;

  STREAM_READ_VAR(ifs, min_x);
  STREAM_READ_VAR(ifs, min_y);
  STREAM_READ_VAR(ifs, max_x);
  STREAM_READ_VAR(ifs, max_y);

  pgraph->bb.min_corner().x(min_x);
  pgraph->bb.min_corner().y(min_y);
  pgraph->bb.max_corner().x(max_x);
  pgraph->bb.max_corner().y(max_y);

  STREAM_READ_VAR(ifs, check_sum);
  if (check_sum != 0xabcdabcd) {
    std::cerr << "Bad checksum " << path << std::endl;
    abort();
  }

  ifs.close();

  std::cout << "Map " << path
            << " is deserialized, chains: " << pgraph->chains.size()
            << " points: " << pgraph->points.size()
            << " edges: " << pgraph->points.size() - pgraph->chains.size()
            << std::endl;
  return pgraph;
}

template <typename COORD_T>
std::shared_ptr<PlanarGraph<COORD_T>>
load_from(const std::string &path, const std::string &serialize_prefix) {
  std::string escaped_path;
  std::replace_copy(path.begin(), path.end(), std::back_inserter(escaped_path),
                    '/', '-');
  if (!serialize_prefix.empty()) {
    DIR *dir = opendir(serialize_prefix.c_str());
    if (dir) {
      closedir(dir);
    } else if (ENOENT == errno) {
      if (mkdir(serialize_prefix.c_str(), 0755)) {
        std::cerr << "Cannot create dir " << path << std::endl;
        abort();
      }
    } else {
      std::cerr << "Cannot open dir " << path << std::endl;
      abort();
    }
  }

  auto ser_path = serialize_prefix + '/' + escaped_path + ".bin";

  if (access(ser_path.c_str(), R_OK) == 0) {
    return deserialize_pgraph<COORD_T>(ser_path.c_str());
  }
  auto pgraph = read_pgraph<COORD_T>(path.c_str());
  if (!serialize_prefix.empty() &&
      access(serialize_prefix.c_str(), W_OK) == 0) {
    serialize_pgraph<COORD_T>(pgraph, ser_path.c_str());
  }
  return pgraph;
}

template <typename COEFFICIENT_T> struct Edge {
  uint32_t eid;
  uint32_t p1_idx, p2_idx;
  COEFFICIENT_T a, b, c; // ax + by + c=0; b >= 0
};

template <typename COORD_T>
void ExtractLineSegs(const std::shared_ptr<PlanarGraph<COORD_T>> &pgraph,
                     std::vector<COORD_T> &points_x,
                     std::vector<COORD_T> &points_y,
                     std::vector<Edge<COORD_T>> &edges) {
  points_x.resize(pgraph->points.size());
  points_y.resize(pgraph->points.size());

  for (size_t i = 0; i < pgraph->points.size(); i++) {
    points_x[i] = pgraph->points[i].x();
    points_y[i] = pgraph->points[i].y();
  }

  edges.resize(pgraph->points.size() - pgraph->chains.size());

  for (size_t ichain = 0; ichain < pgraph->chains.size(); ichain++) {
    const auto &chain = pgraph->chains[ichain];

    for (auto p_idx = pgraph->row_index[ichain];
         p_idx < pgraph->row_index[ichain + 1] - 1; p_idx++) {
      auto eid = p_idx - ichain;
      auto &e = edges[eid];

      e.eid = eid;
      e.p1_idx = p_idx;
      e.p2_idx = p_idx + 1;

      auto x1 = points_x[e.p1_idx];
      auto y1 = points_y[e.p1_idx];
      auto x2 = points_x[e.p2_idx];
      auto y2 = points_y[e.p2_idx];

      e.a = y1 - y2;
      e.b = x2 - x1;
      e.c = x1 * e.a - y1 * e.b;

      assert(e.a != 0 || e.b != 0);

      if (e.b < 0) {
        e.a = -e.a;
        e.b = -e.b;
        e.c = -e.c;
      }
    }
  }
}

#endif // SPATIALQUERYBENCHMARK_CBD_LOADER_H
