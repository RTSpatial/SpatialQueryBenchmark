#!/usr/bin/env bash

export DATASET_ROOT="/local/storage/liang/rtspatial/datasets"
export SERIALIZE_ROOT="/local/storage/liang/rtspatial/ser"
export QUERY_ROOT="${DATASET_ROOT}/queries"
export BENCHMARK_ROOT="/local/storage/liang/.clion/SpatialQueryBenchmark/cmake-build-release-dl190"
export DATASET_WKT_FILES=("dtl_cnty.wkt"
  "USACensusBlockGroupBoundaries.wkt"
  "USADetailedWaterBodies.wkt"
  "parks_Europe.wkt")
export CONTAINS_QUERY_SIZE=100000
export INTERSECTS_QUERY_SIZE=10000
export QUERY_VARY_SIZES=(50000 100000 200000 400000 800000)
export RANGE_QUERY_INTERSECTS_QUALIFIED_SIZES=(1 10 100)
export RANGE_QUERY_INTERSECTS_SELECTIVITIES=("0.0001" "0.001" "0.01")
export RANGE_QUERY_INTERSECTS_LOAD_FACTORS=("0.01" "0.03" "0.5")
