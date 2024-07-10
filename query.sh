#!/usr/bin/env bash

./cmake-build-release/query -geom /Users/liang/polygon_wkt/dtl_cnty/dtl_cnty.wkt \
  -query ./dtl_cnty_contains_query.wkt \
  -query_type "range-contains" \
  -index_type "rtree" \
  -limit 100

./cmake-build-release/query -geom /Users/liang/polygon_wkt/dtl_cnty/dtl_cnty.wkt \
  -query ./dtl_cnty_intersects_query.wkt \
  -query_type "range-intersects" \
  -index_type "rtree" \
  -limit 100

./cmake-build-release/query -geom /Users/liang/polygon_wkt/dtl_cnty/dtl_cnty.wkt \
  -query ./dtl_cnty_point_query.wkt \
  -query_type "point-contains" \
  -index_type "rtree" \
  -limit 100