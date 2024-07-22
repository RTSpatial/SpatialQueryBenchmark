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

FAIL=0

for dist in uniform gaussian; do
  for n in 1000000 2000000 3000000 4000000 5000000; do
    # generate polys about the center of USA
    out_wkt="${DATASET_ROOT}/synthetic/${dist}_n_${n}.wkt"
    if [[ ! -f "$out_wkt" ]]; then
      ./generator.py distribution=$dist \
        cardinality=$n \
        dimensions=2 \
        seed=1 \
        geometry=box \
        polysize=0.01 \
        maxseg=3 \
        format=wkt \
        affinematrix=1,0,0,0,1,0 \
        maxsize=0.01,0.01 \
        affinematrix=1,0,0,0,1,0 >"$out_wkt"
    fi
  done
done

for job in $(jobs -p); do
  echo $job
  wait $job || let "FAIL+=1"
done

echo $FAIL

if [ "$FAIL" == "0" ]; then
  echo "YAY!"
else
  echo "FAIL! ($FAIL)"
fi
