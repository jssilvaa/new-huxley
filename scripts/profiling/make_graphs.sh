#!/usr/bin/env bash
set -euo pipefail

# Convert perf.data into:
# - perf.out (perf script output)
# - out.folded (collapsed stacks)
# - flamegraph.full.svg (all frames)
# - flamegraph.app.svg (filtered to stacks touching (appchat))
# - perf.report.full.txt and perf.report.app.txt (top tables)

if [[ $# -lt 1 ]]; then
  echo "Usage: $0 <perf.data> [flamegraph_dir]" >&2
  exit 1
fi

PERF_DATA="$1"
FLAMEGRAPH_DIR="${2:-$HOME/FlameGraph}"

if [[ ! -f "$PERF_DATA" ]]; then
  echo "perf.data not found: $PERF_DATA" >&2
  exit 1
fi

SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"

# We `cd` into the output directory below; make PERF_DATA absolute so `perf -i` keeps working.
if command -v realpath >/dev/null 2>&1; then
  PERF_DATA="$(realpath "$PERF_DATA")"
else
  PERF_DATA="$(cd "$(dirname "$PERF_DATA")" && pwd)/$(basename "$PERF_DATA")"
fi

command -v perf >/dev/null 2>&1 || { echo "Missing perf" >&2; exit 1; }

STACKCOLLAPSE="$FLAMEGRAPH_DIR/stackcollapse-perf.pl"
FLAMEGRAPH="$FLAMEGRAPH_DIR/flamegraph.pl"

if [[ ! -x "$STACKCOLLAPSE" || ! -x "$FLAMEGRAPH" ]]; then
  echo "FlameGraph scripts not found in $FLAMEGRAPH_DIR" >&2
  echo "Install:" >&2
  echo "  git clone https://github.com/brendangregg/FlameGraph.git $FLAMEGRAPH_DIR" >&2
  exit 1
fi

OUT_DIR="$(cd "$(dirname "$PERF_DATA")" && pwd)"
cd "$OUT_DIR"

echo "Generating perf.out..."
perf script -i "$PERF_DATA" -F comm,pid,tid,time,ip,sym,dso > perf.out

echo "Collapsing stacks..."
"$STACKCOLLAPSE" perf.out > out.folded

echo "Generating flamegraph.full.svg..."
"$FLAMEGRAPH" out.folded > flamegraph.full.svg

# Filter folded stacks to those that include frames from the app binary.
# perf script typically annotates symbols with (appchat). If it doesn't, the
# app-only flamegraph may end up empty (full.svg will still be valid).
echo "Generating flamegraph.app.svg (filtered)..."
# keep stacks where the command/comm at the start is appchat
grep -E '^appchat;|;appchat\b' out.folded > out.app.folded || true
if [[ -s out.app.folded ]]; then
  "$FLAMEGRAPH" out.app.folded > flamegraph.app.svg
else
  echo "Warning: no '(appchat)' frames found; skipping flamegraph.app.svg" >&2
fi

echo "Generating perf.report.*.txt..."
# Full report
perf report --stdio -i "$PERF_DATA" --no-children --sort comm,dso,symbol > perf.report.full.txt || true
# App-focused report
perf report --stdio -i "$PERF_DATA" --no-children --dsos=appchat --sort comm,dso,symbol > perf.report.app.txt || true

echo "Generating LaTeX tables (top N)..."
: "${PERF_TOP_N:=5}"
LATEX_SCRIPT="$SCRIPT_DIR/perf_report_to_latex.py"
if command -v python3 >/dev/null 2>&1 && [[ -f "$LATEX_SCRIPT" ]]; then
  gen_tex() {
    local in_file="$1"
    local out_file="$2"
    local tmp_file
    tmp_file="${out_file}.tmp"
    if python3 "$LATEX_SCRIPT" "$in_file" --top "$PERF_TOP_N" --table > "$tmp_file"; then
      mv -f "$tmp_file" "$out_file"
    else
      rm -f "$tmp_file"
      return 1
    fi
  }

  gen_tex perf.report.full.txt "perf.top${PERF_TOP_N}.full.tex" || true
  # App report can legitimately have 0 rows (e.g. startup dominated by Qt/libs)
  if grep -qE '^[[:space:]]*[0-9]+(\.[0-9]+)?%' perf.report.app.txt 2>/dev/null; then
    gen_tex perf.report.app.txt "perf.top${PERF_TOP_N}.app.tex" || true
  fi
else
  echo "Warning: python3 or perf_report_to_latex.py not found; skipping LaTeX tables" >&2
fi

echo "Done. Outputs in: $OUT_DIR"
ls -1 perf.out out.folded flamegraph.full.svg flamegraph.app.svg perf.report.full.txt perf.report.app.txt perf.top*.tex 2>/dev/null || true
