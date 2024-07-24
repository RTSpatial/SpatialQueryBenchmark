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

function run_range_query_intersects() {
  query_type="range-intersects"
  index_type="rtspatial-vary-parallelism"

  for wkt_file in "${DATASET_WKT_FILES[@]}"; do
    for ((i = 0; i < ${#RANGE_QUERY_INTERSECTS_SELECTIVITIES[@]}; i++)); do
      selectivity=${RANGE_QUERY_INTERSECTS_SELECTIVITIES[$i]}
      query_dir="${QUERY_ROOT}/${query_type}_select_${selectivity}_queries_${QUERY_SIZE}"
      query="${query_dir}/${wkt_file}"
      log="${log_dir}/${query_type}_select_${selectivity}_queries_${QUERY_SIZE}/${index_type}/${wkt_file}.log"

      if [[ ! -f "${log}" || true ]]; then
        echo "$log" | xargs dirname | xargs mkdir -p

        echo "Running query $query"
        cmd="${BENCHMARK_ROOT}/query -geom ${DATASET_ROOT}/polygons/${wkt_file} \
          -query $query \
          -serialize $SERIALIZE_ROOT \
          -query_type ${query_type} \
          -index_type $index_type \
          -parallelism 512 \
          -avg_time=false"

        echo "$cmd" >"${log}.tmp"
        eval "$cmd" 2>&1 | tee -a "${log}.tmp"

        if grep -q "Query Time" "${log}.tmp"; then
          mv "${log}.tmp" "${log}"
        fi
      fi
    done
  done
}

run_range_query_intersects
