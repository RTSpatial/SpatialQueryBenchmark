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
  query_type="point-contains"
  index_type="$1"
  for wkt_file in "${DATASET_WKT_FILES[@]}"; do
    query_dir="${QUERY_ROOT}/${query_type}_queries_${CONTAINS_QUERY_SIZE}"
    query="${query_dir}/${wkt_file}"
    log="${log_dir}/${query_type}_queries_${CONTAINS_QUERY_SIZE}/${index_type}/${wkt_file}.log"

    if [[ ! -f "${log}" ]]; then
      echo "${log}" | xargs dirname | xargs mkdir -p

      echo "Running query $query"
      if [[ $index_type == "cuspatial" ]]; then
        cmd="python3 ${script_dir}/cuspatial_point_contains.py ${DATASET_ROOT}/polygons/${wkt_file} $query"
      else
        cmd="$BENCHMARK_ROOT/query -geom ${DATASET_ROOT}/polygons/${wkt_file} \
        -query $query \
        -serialize $SERIALIZE_ROOT \
        -query_type $query_type \
        -index_type $index_type \
        -load_factor 0.001"
      fi

      echo "$cmd" >"${log}.tmp"
      eval "$cmd" 2>&1 | tee -a "${log}.tmp"

      if grep -q "Query Time" "${log}.tmp"; then
        mv "${log}.tmp" "${log}"
      fi
    fi
  done
}

function run_point_query_contains_vary_size() {
  query_type="point-contains"
  index_type="$1"
  wkt_file="parks_Europe.wkt"

  for query_size in "${QUERY_VARY_SIZES_CONTAINS[@]}"; do
    query_dir="${QUERY_ROOT}/${query_type}_queries_${query_size}"
    query="${query_dir}/${wkt_file}"
    log="${log_dir}/${query_type}_queries_${query_size}/${index_type}/${wkt_file}.log"

    if [[ ! -f "${log}" ]]; then
      echo "${log}" | xargs dirname | xargs mkdir -p

      echo "Running query $query"
      if [[ $index_type == "cuspatial" ]]; then
        cmd="python3 ${script_dir}/cuspatial_point_contains.py ${DATASET_ROOT}/polygons/${wkt_file} $query"
      else
        cmd="$BENCHMARK_ROOT/query -geom ${DATASET_ROOT}/polygons/${wkt_file} \
        -query $query \
        -serialize $SERIALIZE_ROOT \
        -query_type $query_type \
        -index_type $index_type \
        -load_factor 0.0001"
      fi

      echo "$cmd" >"${log}.tmp"
      eval "$cmd" 2>&1 | tee -a "${log}.tmp"

      if grep -q "Query Time" "${log}.tmp"; then
        mv "${log}.tmp" "${log}"
      fi
    fi
  done
}

function run_range_query_contains() {
  query_type="range-contains"
  index_type="$1"
  for wkt_file in "${DATASET_WKT_FILES[@]}"; do
    query_dir="${QUERY_ROOT}/${query_type}_queries_${CONTAINS_QUERY_SIZE}"
    query="${query_dir}/${wkt_file}"
    log="${log_dir}/${query_type}_queries_${CONTAINS_QUERY_SIZE}/${index_type}/${wkt_file}.log"

    if [[ ! -f "${log}" ]]; then
      echo "$log" | xargs dirname | xargs mkdir -p

      echo "Running query $query"
      cmd="$BENCHMARK_ROOT/query -geom ${DATASET_ROOT}/polygons/${wkt_file} \
        -query $query \
        -serialize $SERIALIZE_ROOT \
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

function run_range_query_contains_vary_size() {
  query_type="range-contains"
  index_type="$1"
  wkt_file="parks_Europe.wkt"

  for query_size in "${QUERY_VARY_SIZES_CONTAINS[@]}"; do
    query_dir="${QUERY_ROOT}/${query_type}_queries_${query_size}"
    query="${query_dir}/${wkt_file}"
    log="${log_dir}/${query_type}_queries_${query_size}/${index_type}/${wkt_file}.log"

    if [[ ! -f "${log}" ]]; then
      echo "${log}" | xargs dirname | xargs mkdir -p

      echo "Running query $query"
      cmd="$BENCHMARK_ROOT/query -geom ${DATASET_ROOT}/polygons/${wkt_file} \
        -query $query \
        -serialize $SERIALIZE_ROOT \
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

function run_range_query_intersects() {
  query_type="range-intersects"
  index_type="$1"
  for wkt_file in "${DATASET_WKT_FILES[@]}"; do
    for ((i = 0; i < ${#RANGE_QUERY_INTERSECTS_SELECTIVITIES[@]}; i++)); do
      selectivity=${RANGE_QUERY_INTERSECTS_SELECTIVITIES[$i]}
      query_dir="${QUERY_ROOT}/${query_type}_select_${selectivity}_queries_${INTERSECTS_QUERY_SIZE}"
      query="${query_dir}/${wkt_file}"
      log="${log_dir}/${query_type}_select_${selectivity}_queries_${INTERSECTS_QUERY_SIZE}/${index_type}/${wkt_file}.log"

      if [[ ! -f "${log}" ]]; then
        echo "$log" | xargs dirname | xargs mkdir -p

        echo "Running query $query"
        cmd="${BENCHMARK_ROOT}/query -geom ${DATASET_ROOT}/polygons/${wkt_file} \
          -query $query \
          -serialize $SERIALIZE_ROOT \
          -query_type ${query_type} \
          -index_type $index_type" # park_europe, rays=50

        echo "$cmd" >"${log}.tmp"
        eval "$cmd" 2>&1 | tee -a "${log}.tmp"

        if grep -q "Query Time" "${log}.tmp"; then
          mv "${log}.tmp" "${log}"
        fi
      fi
    done
  done
}


function run_range_query_intersects_vary_size() {
  query_type="range-intersects"
  index_type="$1"
  wkt_file="parks_Europe.wkt"
  selectivity="0.001"

  for query_size in "${QUERY_VARY_SIZES_INTERSECTS[@]}"; do
    query_dir="${QUERY_ROOT}/${query_type}_select_${selectivity}_queries_${query_size}"
    query="${query_dir}/${wkt_file}"
    log="${log_dir}/${query_type}_select_${selectivity}_queries_${query_size}/${index_type}/${wkt_file}.log"

    if [[ ! -f "${log}" ]]; then
      echo "${log}" | xargs dirname | xargs mkdir -p

      echo "Running query $query"
      cmd="$BENCHMARK_ROOT/query -geom ${DATASET_ROOT}/polygons/${wkt_file} \
        -query $query \
        -serialize $SERIALIZE_ROOT \
        -query_type $query_type \
        -index_type $index_type \
        -load_factor 0.01"

      echo "$cmd" >"${log}.tmp"
      eval "$cmd" 2>&1 | tee -a "${log}.tmp"

      if grep -q "Query Time" "${log}.tmp"; then
        mv "${log}.tmp" "${log}"
      fi
    fi
  done
}

function vary_parallelism_range_query_intersects() {
  query_type="range-intersects"
  index_type="rtspatial-vary-parallelism"

  for wkt_file in "${DATASET_WKT_FILES[@]}"; do
      selectivity="0.0001"
      query_dir="${QUERY_ROOT}/${query_type}_select_${selectivity}_queries_${RAY_DUP_INTERSECTS_QUERY_SIZE}"
      query="${query_dir}/${wkt_file}"
      log="${log_dir}/ray_duplication_${query_type}_select_${selectivity}_queries_${RAY_DUP_INTERSECTS_QUERY_SIZE}/${index_type}/${wkt_file}.log"

      if [[ ! -f "${log}" ]]; then
        echo "$log" | xargs dirname | xargs mkdir -p

        echo "Running query $query"
        cmd="${BENCHMARK_ROOT}/query -geom ${DATASET_ROOT}/polygons/${wkt_file} \
          -query $query \
          -serialize $SERIALIZE_ROOT \
          -query_type ${query_type} \
          -index_type $index_type \
          -parallelism 512 \
          -avg_time=false \
          -load_factor=0.8"

        echo "$cmd" >"${log}.tmp"
        eval "$cmd" 2>&1 | tee -a "${log}.tmp"

        if grep -q "Query Time" "${log}.tmp"; then
          mv "${log}.tmp" "${log}"
        fi
      fi
  done
}

# CPU-based "rtree" "cgal"
# GPU-based "cuspatial" "lbvh" "rtspatial"

for index_type in "cuspatial" "lbvh" "rtspatial"; do
  run_point_query_contains "$index_type"
done

for index_type in "cuspatial" "lbvh" "rtspatial"; do
  run_point_query_contains_vary_size "$index_type"
done

# CPU-based "rtree" "glin"
# GPU-based "cuspatial" "lbvh" "rtspatial"

for index_type in "lbvh" "rtspatial"; do
  run_range_query_contains "$index_type"
  run_range_query_intersects "$index_type"
  run_range_query_intersects_vary_size "$index_type"
done

for index_type in "lbvh" "rtspatial"; do
  run_range_query_contains_vary_size "$index_type"
done

#vary_parallelism_range_query_intersects
