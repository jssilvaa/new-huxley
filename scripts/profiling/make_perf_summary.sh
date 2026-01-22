#!/usr/bin/env bash
set -euo pipefail

# perf summary output

SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
ROOT_DIR="$(cd "$SCRIPT_DIR/../.." && pwd)"

: "${PERF_BASE:=$ROOT_DIR/profiles/perf}"
: "${OUT_DIR:=$PERF_BASE}"  # output base dir
: "${MODE:=full}"           # full or app
: "${TOP:=10}"
: "${FORMAT:=latex}"        # latex or csv
: "${TABLE:=1}"             # latex table

args=("$PERF_BASE" --all --mode "$MODE" --top "$TOP" --out-dir "$OUT_DIR")

if [[ "$FORMAT" == "csv" ]]; then
  args+=(--csv)
else
  args+=(--latex)
  if [[ "$TABLE" == "1" ]]; then
    args+=(--table)
  fi
fi

python3 "$SCRIPT_DIR/perf_aggregate.py" "${args[@]}"

echo "Wrote aggregated outputs to: $OUT_DIR"
if [[ "$FORMAT" == "csv" ]]; then
  ls -1 "$OUT_DIR"/perf.mean.*."$MODE".csv 2>/dev/null || true
else
  ls -1 "$OUT_DIR"/perf.mean.*."$MODE".tex 2>/dev/null || true
fi
