#!/usr/bin/env bash

./cmake-build-release/query -geom /Users/liang/polygon_wkt/dtl_cnty/dtl_cnty.wkt \
  -query ./dtl_cnty_contains_query.wkt \
  -query_type "range-contains" \
  -index_type "rtree" \
  -limit 100