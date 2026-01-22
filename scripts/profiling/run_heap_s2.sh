#!/usr/bin/env bash
set -euo pipefail

# heaptrack scenario 2

SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
# shellcheck source=common_mem.sh
source "$SCRIPT_DIR/common_mem.sh"

require_cmd heaptrack
require_cmd python3

: "${HEAP_LATEX:=0}"
: "${HEAP_LATEX_CAPTION:=Heaptrack Scenario 2 (Scroll + Typing)}"
: "${HEAP_LATEX_LABEL:=tab:heap-s2}"

scenario="heap_s2_scroll_typing"
out_dir="$(ensure_out_dir "${scenario}_$(timestamp)")"
app="$(resolve_app)"

# heaptrack output prefix
OUT_PREFIX="$out_dir/heaptrack"

cat <<EOF
=== Heaptrack: Scenario 2 (Scroll + Typing) ===
Goal: find allocation churn / hotspots.

Setup:
- Ensure a chat has a few hundred messages.

Steps:
- Start the app.
- Open chat view.
- Scroll up/down for ~15–30 seconds.
- Type a few words.
- Close the app.
EOF

print_instructions

cmd=(heaptrack -o "$OUT_PREFIX" -- "$app")
write_meta "$out_dir" "$scenario" "heaptrack" "$app" "${cmd[*]}"

echo "Output: $out_dir"
echo "App:    $app"
echo "Running: ${cmd[*]}"

"${cmd[@]}"

# find heaptrack trace
TRACE="$(ls -1 "$out_dir"/heaptrack*.zst "$out_dir"/heaptrack*.gz 2>/dev/null | head -n 1 || true)"
if [[ -n "$TRACE" ]]; then
  echo "Trace: $TRACE"

  ANALYZE_OUT="$out_dir/heaptrack.analyze.txt"
  # prefer heaptrack analyze
  heaptrack --analyze "$TRACE" > "$ANALYZE_OUT" || true
  if [[ ! -s "$ANALYZE_OUT" ]] && command -v heaptrack_print >/dev/null 2>&1; then
    heaptrack_print "$TRACE" > "$ANALYZE_OUT" || true
  fi

  if [[ -s "$ANALYZE_OUT" ]]; then
    cmd_an=(
      python3 "$SCRIPT_DIR/heaptrack_analyze.py"
      "$ANALYZE_OUT"
      --out-dir "$out_dir"
      --repo-root "$(repo_root)"
      --top 20
      --stack 18
    )
    if [[ "$HEAP_LATEX" == "1" ]]; then
      cmd_an+=(
        --latex
        --latex-caption "$HEAP_LATEX_CAPTION"
        --latex-label "$HEAP_LATEX_LABEL"
        --latex-top 10
      )
    fi
    "${cmd_an[@]}" || true
  fi
fi

echo "Done."
echo "Artifacts:"
ls -1 "$out_dir" | sed 's/^/  - /'
