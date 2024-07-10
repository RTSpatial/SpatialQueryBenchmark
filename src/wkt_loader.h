#ifndef SPATIALQUERYBENCHMARK_LOADER_H
#define SPATIALQUERYBENCHMARK_LOADER_H
#include <fstream>
#include <vector>

#include <boost/geometry.hpp>
#include <boost/geometry/geometries/linestring.hpp>
#include <boost/geometry/geometries/point_xy.hpp>
#include <boost/geometry/geometries/polygon.hpp>
#include <boost/geometry/geometries/register/box.hpp>
#include <boost/geometry/geometries/register/point.hpp>

#include "common.h"

std::vector<box_t> LoadBoxes(const std::string &path,
                             int limit = std::numeric_limits<int>::max()) {
  std::ifstream ifs(path);
  std::string line;
  std::vector<box_t> boxes;

  while (std::getline(ifs, line)) {
    if (!line.empty()) {
      std::vector<point_t> points;

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
      } else {
        std::cerr << "Bad Geometry " << line << "\n";
        abort();
      }

      double lows[2] = {std::numeric_limits<double>::max(),
                        std::numeric_limits<double>::max()};
      double highs[2] = {std::numeric_limits<double>::lowest(),
                         std::numeric_limits<double>::lowest()};

      for (auto &p : points) {
        lows[0] = std::min(lows[0], p.x());
        highs[0] = std::max(highs[0], p.x());
        lows[1] = std::min(lows[1], p.y());
        highs[1] = std::max(highs[1], p.y());
      }

      box_t box(point_t(lows[0], lows[1]), point_t(highs[0], highs[1]));

      boxes.push_back(box);
      if (boxes.size() % 1000 == 0) {
        std::cout << "Loaded geometries " << boxes.size() / 1000 << " K"
                  << std::endl;
      }
      if (boxes.size() >= limit) {
        break;
      }
    }
  }
  ifs.close();
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
      if (points.size() % 1000 == 0) {
        std::cout << "Loaded geometries " << points.size() / 1000 << std::endl;
      }
      if (points.size() >= limit) {
        break;
      }
    }
  }
  ifs.close();
  return points;
}

#endif // SPATIALQUERYBENCHMARK_LOADER_H
