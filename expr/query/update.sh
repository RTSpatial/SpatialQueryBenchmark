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

function run_update() {
  op=$1
  for wkt_file in "uniform_n_50000000.wkt" "gaussian_n_50000000.wkt"; do
    log="${log_dir}/$op/${wkt_file}.log"

    if [[ ! -f "${log}" ]]; then
      echo "${log}" | xargs dirname | xargs mkdir -p

      cmd="$BENCHMARK_ROOT/query -geom ${DATASET_ROOT}/synthetic/${wkt_file} \
        -serialize $SERIALIZE_ROOT \
        -query_type $op \
        -index_type rtspatial"

      echo "$cmd" >"${log}.tmp"
      eval "$cmd" 2>&1 | tee -a "${log}.tmp"

      if grep -q "Time" "${log}.tmp"; then
        mv "${log}.tmp" "${log}"
      fi
    fi
  done
}

function run_update_batch() {
  op=$1
  for wkt_file in "uniform_n_50000000.wkt" "gaussian_n_50000000.wkt"; do
    for batch in "${BATCH_SIZES[@]}"; do
      log="${log_dir}/${op}_batch_${batch}/${wkt_file}.log"

      if [[ ! -f "${log}" ]]; then
        echo "${log}" | xargs dirname | xargs mkdir -p

        cmd="$BENCHMARK_ROOT/query -geom ${DATASET_ROOT}/synthetic/${wkt_file} \
        -serialize $SERIALIZE_ROOT \
        -query_type $op \
        -index_type rtspatial \
        -batch $batch"

        echo "$cmd" >"${log}.tmp"
        eval "$cmd" 2>&1 | tee -a "${log}.tmp"

        if grep -q "Time" "${log}.tmp"; then
          mv "${log}.tmp" "${log}"
        fi
      fi
    done
  done
}

run_update "insertion"
run_update "deletion"

run_update_batch "insertion"
run_update_batch "deletion"
