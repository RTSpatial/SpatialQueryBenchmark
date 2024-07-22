#!/usr/bin/env bash

# Function to resolve the script path
get_script_dir() {
  local source="${BASH_SOURCE[0]}"
  while [ -h "$source" ]; do
    local dir
    dir=$(dirname "$source")
    source=$(readlink "$source")
    [[ $source != /* ]] && source="$dir/$source"
  done
  echo "$(cd -P "$(dirname "$source")" >/dev/null 2>&1 && pwd)"
}
script_dir=$(get_script_dir)

source "${script_dir}/../common.sh"

log_dir="${script_dir}/logs"

function run_point_query_contains() {
  #  DATASET_WKT_FILES=("dtl_cnty.wkt")
  query_type="point-contains"
  index_type="$1"
  for wkt_file in "${DATASET_WKT_FILES[@]}"; do
    query_dir="${QUERY_ROOT}/${query_type}_queries_${QUERY_SIZE}"
    query="${query_dir}/${wkt_file}"
    log="${log_dir}/${query_type}_${query_type}_queries_${QUERY_SIZE}/${index_type}/${wkt_file}.log"

    if [[ ! -f "${log}" ]]; then
      echo "${log}" | xargs dirname | xargs mkdir -p

      echo "Running query $query"
      cmd="${BENCHMARK_ROOT}/query -geom ${DATASET_ROOT}/polygons/${wkt_file} \
        -query $query \
        -query_type $query_type \
        -index_type $index_type \
        -load_factor 0.001"

      echo "$cmd" >"${log}.tmp"
      eval "$cmd" 2>&1 | tee -a "${log}.tmp"

      if grep -q "Query Time" "${log}.tmp"; then
        mv "${log}.tmp" "${log}"
      fi
    fi
  done
}

function run_range_query_contains() {
  #  DATASET_WKT_FILES=("dtl_cnty.wkt")
  query_type="range-contains"
  index_type="$1"
  for wkt_file in "${DATASET_WKT_FILES[@]}"; do
    query_dir="${QUERY_ROOT}/${query_type}_queries_${QUERY_SIZE}"
    query="${query_dir}/${wkt_file}"
    log="${log_dir}/${query_type}_queries_${QUERY_SIZE}/${index_type}/${wkt_file}.log"

    if [[ ! -f "${log}" ]]; then
      echo "$log" | xargs dirname | xargs mkdir -p

      echo "Running query $query"
      cmd="$BENCHMARK_ROOT/query -geom ${DATASET_ROOT}/polygons/${wkt_file} \
        -query $query \
        -query_type $query_type \
        -index_type $index_type \
        -load_factor 0.001"
      echo "$cmd" >"${log}.tmp"
      eval "$cmd" 2>&1 | tee -a "${log}.tmp"

      if grep -q "Query Time" "${log}.tmp"; then
        mv "${log}.tmp" "${log}"
      fi
    fi
  done
}
#
#function run_range_query_intersects() {
#  query_type="range-intersects"
#  index_type="$1"
#  for wkt_file in "${DATASET_WKT_FILES[@]}"; do
#    for qualified_size in "${RANGE_QUERY_INTERSECTS_QUALIFIED_SIZES[@]}"; do
#      query_dir="${QUERY_ROOT}/${query_type}_qualified_${qualified_size}_queries_${QUERY_SIZE}"
#      query="${query_dir}/${wkt_file}"
#      log="${log_dir}/${query_type}_qualified_${qualified_size}_queries_${QUERY_SIZE}/${index_type}/${wkt_file}.log"
#
#      if [[ ! -f "${log}" ]]; then
#        echo "$log" | xargs dirname | xargs mkdir -p
#
#        echo "Running query $query"
#        cmd="${BENCHMARK_ROOT}/query -geom ${DATASET_ROOT}/polygons/${wkt_file} \
#          -query $query \
#          -query_type ${query_type} \
#          -index_type $index_type \
#          -load_factor 0.5 \
#          -rays 200" # park_europe, rays=50
#
#        echo "$cmd" >"${log}.tmp"
#        eval "$cmd" 2>&1 | tee -a "${log}.tmp"
#
#        if grep -q "Query Time" "${log}.tmp"; then
#          mv "${log}.tmp" "${log}"
#        fi
#      fi
#    done
#  done
#}

function run_range_query_intersects() {
  query_type="range-intersects"
  index_type="$1"
  for wkt_file in "${DATASET_WKT_FILES[@]}"; do
    for ((i = 0; i < ${#RANGE_QUERY_INTERSECTS_SELECTIVITIES[@]}; i++)); do
      selectivity=${RANGE_QUERY_INTERSECTS_SELECTIVITIES[$i]}
      load_factor=${RANGE_QUERY_INTERSECTS_LOAD_FACTORS[$i]}
      query_dir="${QUERY_ROOT}/${query_type}_select_${selectivity}_queries_${QUERY_SIZE}"
      query="${query_dir}/${wkt_file}"
      log="${log_dir}/${query_type}_select_${selectivity}_queries_${QUERY_SIZE}/${index_type}/${wkt_file}.log"

      if [[ ! -f "${log}" ]]; then
        echo "$log" | xargs dirname | xargs mkdir -p

        echo "Running query $query"
        cmd="${BENCHMARK_ROOT}/query -geom ${DATASET_ROOT}/polygons/${wkt_file} \
          -query $query \
          -serialize $SERIALIZE_ROOT \
          -query_type ${query_type} \
          -index_type $index_type \
          -load_factor $load_factor \
          -rays 200" # park_europe, rays=50

        echo "$cmd" >"${log}.tmp"
        eval "$cmd" 2>&1 | tee -a "${log}.tmp"

        if grep -q "Query Time" "${log}.tmp"; then
          mv "${log}.tmp" "${log}"
        fi
      fi
    done
  done
}

for index_type in "rtree" "rtree-parallel" "rtspatial"; do
  run_point_query_contains "$index_type"
  run_range_query_contains "$index_type"
  run_range_query_intersects "$index_type"
done

#run_range_query_intersects "rtree"

#run_query_contains "range-intersects"
#./cmake-build-release/query -geom /Users/liang/polygon_wkt/dtl_cnty/dtl_cnty.wkt \
#  -query ./dtl_cnty_contains_query.wkt \
#  -query_type "range-contains" \
#  -index_type "rtree" \
#  -limit 100
#
#./cmake-build-release/query -geom /Users/liang/polygon_wkt/dtl_cnty/dtl_cnty.wkt \
#  -query ./dtl_cnty_intersects_query.wkt \
#  -query_type "range-intersects" \
#  -index_type "rtree" \
#  -limit 100
#
#./cmake-build-release/query -geom /Users/liang/polygon_wkt/dtl_cnty/dtl_cnty.wkt \
#  -query ./dtl_cnty_point_query.wkt \
#  -query_type "point-contains" \
#  -index_type "rtree" \
#  -limit 100
