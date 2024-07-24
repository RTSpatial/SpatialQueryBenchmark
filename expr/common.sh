#!/usr/bin/env bash

export DATASET_ROOT="/local/storage/liang/rtspatial/datasets"
export SERIALIZE_ROOT="/local/storage/liang/rtspatial/datasets/ser"
export QUERY_ROOT="${DATASET_ROOT}/queries"
export BENCHMARK_ROOT="/local/storage/liang/.clion/SpatialQueryBenchmark/cmake-build-release-dl190"
export DATASET_WKT_FILES=("dtl_cnty.wkt"
  "USACensusBlockGroupBoundaries.wkt"
  "USADetailedWaterBodies.wkt"
  "parks_Europe.wkt")
export QUERY_SIZE=100000
export RANGE_QUERY_INTERSECTS_QUALIFIED_SIZES=(1 10 100)
export RANGE_QUERY_INTERSECTS_SELECTIVITIES=("0.0001" "0.001" "0.01")
export RANGE_QUERY_INTERSECTS_LOAD_FACTORS=("0.01" "0.03" "0.5")
