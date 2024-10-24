#ifndef SPATIALQUERYBENCHMARK_WKT_LOADER_H
#define SPATIALQUERYBENCHMARK_WKT_LOADER_H
#include <dirent.h>
#include <sys/stat.h>

#include <boost/archive/binary_iarchive.hpp>
#include <boost/archive/binary_oarchive.hpp>
#include <boost/geometry.hpp>
#include <boost/geometry/geometries/linestring.hpp>
#include <boost/geometry/geometries/point_xy.hpp>
#include <boost/geometry/geometries/polygon.hpp>
#include <boost/geometry/geometries/register/box.hpp>
#include <boost/geometry/geometries/register/point.hpp>
#include <boost/geometry/geometries/ring.hpp>
#include <boost/serialization/vector.hpp>
#include <fstream>
#include <vector>

#include "geom_common.h"

std::vector<polygon_t>
LoadPolygons(const std::string &path,
             int limit = std::numeric_limits<int>::max()) {
  std::ifstream ifs(path);
  std::string line;
  std::vector<polygon_t> polygons;

  while (std::getline(ifs, line)) {
    if (!line.empty()) {
      if (line.rfind("MULTIPOLYGON", 0) == 0) {
        boost::geometry::model::multi_polygon<polygon_t> multi_poly;
        boost::geometry::read_wkt(line, multi_poly);

        for (auto &poly : multi_poly) {
          polygons.push_back(poly);
        }
      } else if (line.rfind("POLYGON", 0) == 0) {
        polygon_t poly;
        boost::geometry::read_wkt(line, poly);
        polygons.push_back(poly);
      } else {
        std::cerr << "Bad Geometry " << line << "\n";
        abort();
      }
      if (polygons.size() >= limit) {
        break;
      }
    }
  }
  ifs.close();
  return polygons;
}

std::vector<box_t> PolygonsToBoxes(const std::vector<polygon_t> &polygons) {
  std::vector<box_t> boxes;

  auto ring_points_to_bbox = [&boxes](const std::vector<point_t> &points) {
    coord_t lows[2] = {std::numeric_limits<coord_t>::max(),
                       std::numeric_limits<coord_t>::max()};
    coord_t highs[2] = {std::numeric_limits<coord_t>::lowest(),
                        std::numeric_limits<coord_t>::lowest()};

    for (auto &p : points) {
      lows[0] = std::min(lows[0], p.x());
      highs[0] = std::max(highs[0], p.x());
      lows[1] = std::min(lows[1], p.y());
      highs[1] = std::max(highs[1], p.y());
    }

    box_t box(point_t(lows[0], lows[1]), point_t(highs[0], highs[1]));

    boxes.push_back(box);
  };

  for (auto &poly : polygons) {
    std::vector<point_t> points;

    for (auto &p : poly.outer()) {
      points.push_back(p);
    }
    ring_points_to_bbox(points);
  }
  return boxes;
}

std::vector<point_t> LoadPoints(const std::string &path,
                                int limit = std::numeric_limits<int>::max()) {
  std::ifstream ifs(path);
  std::string line;
  std::vector<point_t> points;

  while (std::getline(ifs, line)) {
    if (!line.empty()) {
      if (line.rfind("MULTIPOLYGON", 0) == 0) {
        boost::geometry::model::multi_polygon<polygon_t> multi_poly;
        boost::geometry::read_wkt(line, multi_poly);

        for (auto &poly : multi_poly) {
          for (auto &p : poly.outer()) {
            points.push_back(p);
          }
        }
      } else if (line.rfind("POLYGON", 0) == 0) {
        polygon_t poly;
        boost::geometry::read_wkt(line, poly);

        for (auto &p : poly.outer()) {
          points.push_back(p);
        }
      } else if (line.rfind("POINT", 0) == 0) {
        point_t p;
        boost::geometry::read_wkt(line, p);

        points.push_back(p);
      } else {
        std::cerr << "Bad Geometry " << line << "\n";
        abort();
      }
      //      if (points.size() % 1000 == 0) {
      //        std::cout << "Loaded geometries " << points.size() / 1000 <<
      //        std::endl;
      //      }
      if (points.size() >= limit) {
        break;
      }
    }
  }
  ifs.close();
  return points;
}

void SerializePolygons(const char *file,
                       const std::vector<polygon_t> &polygons) {
  std::ofstream ofs(file, std::ios::binary);
  boost::archive::binary_oarchive oa(ofs);
  oa << boost::serialization::make_nvp("polygons", polygons);
  ofs.close();
}

std::vector<polygon_t> DeserializePolygons(const char *file) {
  std::vector<polygon_t> deserialized_polygons;
  std::ifstream ifs(file, std::ios::binary);
  boost::archive::binary_iarchive ia(ifs);
  ia >> boost::serialization::make_nvp("polygons", deserialized_polygons);
  ifs.close();
  return deserialized_polygons;
}

void SerializePoints(const char *file, const std::vector<point_t> &points) {
  std::ofstream ofs(file, std::ios::binary);
  boost::archive::binary_oarchive oa(ofs);
  oa << boost::serialization::make_nvp("points", points);
  ofs.close();
}

std::vector<point_t> DeserializePoints(const char *file) {
  std::vector<point_t> deserialized_points;
  std::ifstream ifs(file, std::ios::binary);
  boost::archive::binary_iarchive ia(ifs);
  ia >> boost::serialization::make_nvp("points", deserialized_points);
  ifs.close();
  return deserialized_points;
}

std::vector<point_t> LoadPoints(const std::string &path,
                                const std::string &serialize_prefix,
                                int limit = std::numeric_limits<int>::max()) {
  std::string escaped_path;
  std::replace_copy(path.begin(), path.end(), std::back_inserter(escaped_path),
                    '/', '_');

  if (!serialize_prefix.empty()) {
    DIR *dir = opendir(serialize_prefix.c_str());
    if (dir) {
      closedir(dir);
    } else if (ENOENT == errno) {
      if (mkdir(serialize_prefix.c_str(), 0755)) {
        std::cerr << "Cannot create dir " << path;
        abort();
      }
    } else {
      std::cerr << "Cannot open dir " << path;
      abort();
    }
  }
  auto ser_path = serialize_prefix + "/points_" + escaped_path + "_limit_" +
                  std::to_string(limit) + ".bin";

  std::vector<point_t> points;

  if (access(ser_path.c_str(), R_OK) == 0) {
    points = DeserializePoints(ser_path.c_str());
  } else {
    points = LoadPoints(path, limit);
    if (!serialize_prefix.empty()) {
      SerializePoints(ser_path.c_str(), points);
    }
  }
  return points;
}

std::vector<polygon_t>
LoadPolygons(const std::string &path, const std::string &serialize_prefix,
             int limit = std::numeric_limits<int>::max()) {
  std::string escaped_path;
  std::replace_copy(path.begin(), path.end(), std::back_inserter(escaped_path),
                    '/', '_');

  if (!serialize_prefix.empty()) {
    DIR *dir = opendir(serialize_prefix.c_str());
    if (dir) {
      closedir(dir);
    } else if (ENOENT == errno) {
      if (mkdir(serialize_prefix.c_str(), 0755)) {
        std::cerr << "Cannot create dir " << path;
        abort();
      }
    } else {
      std::cerr << "Cannot open dir " << path;
      abort();
    }
  }
  auto ser_path = serialize_prefix + "/polygon" + escaped_path + "_limit_" +
                  std::to_string(limit) + ".bin";

  std::vector<polygon_t> polygons;

  if (access(ser_path.c_str(), R_OK) == 0) {
    polygons = DeserializePolygons(ser_path.c_str());
  } else {
    polygons = LoadPolygons(path, limit);
    if (!serialize_prefix.empty()) {
      SerializePolygons(ser_path.c_str(), polygons);
    }
  }
  return polygons;
}

namespace boost {
namespace serialization {
template <class Archive>
void serialize(Archive &ar, polygon_t &poly, const unsigned int version) {
  ar &boost::serialization::make_nvp("outer", poly.outer());
  ar &boost::serialization::make_nvp("inners", poly.inners());
}

template <class Archive>
void serialize(Archive &ar, box_t &box, const unsigned int version) {
  ar &boost::serialization::make_nvp("min_corner", box.min_corner());
  ar &boost::serialization::make_nvp("max_corner", box.max_corner());
}

template <class Archive>
void serialize(Archive &ar, boost::geometry::model::ring<point_t> &ring,
               const unsigned int version) {
  serialize(ar, static_cast<std::vector<point_t> &>(ring), version);
}

template <class Archive>
void serialize(Archive &ar, point_t &p, const unsigned int version) {
  ar &const_cast<coord_t &>(p.x());
  ar &const_cast<coord_t &>(p.y());
}

template <class Archive>
void serialize(Archive &ar, line_t &l, const unsigned int version) {
  serialize(ar, l.first, version);
  serialize(ar, l.second, version);
}

} // namespace serialization
} // namespace boost

#endif // SPATIALQUERYBENCHMARK_WKT_LOADER_H
