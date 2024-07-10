#!/usr/bin/env bash

./cmake-build-release/gen -input /Users/liang/polygon_wkt/dtl_cnty/dtl_cnty.wkt \
      -output ./dtl_cnty_intersects_query.wkt \
      -min_qualified 10 \
      -num_queries 1000 \
      -query_type "range-intersects" \
      -limit 10


./cmake-build-release/gen -input /Users/liang/polygon_wkt/dtl_cnty/dtl_cnty.wkt \
      -output ./dtl_cnty_contains_query.wkt \
      -min_qualified 10 \
      -num_queries 1000 \
      -query_type "range-contains" \
      -limit 10


./cmake-build-release/gen -input /Users/liang/polygon_wkt/dtl_cnty/dtl_cnty.wkt \
      -output ./dtl_cnty_point_query.wkt \
      -num_queries 1000 \
      -query_type "point-contains" \
      -limit 10