#!/usr/bin/env bash
set -euo pipefail

# scenario 2 scroll typing

SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
# shellcheck source=common.sh
source "$SCRIPT_DIR/common.sh"

scenario="s2_scroll_typing"
out_dir="$(ensure_out_dir "${scenario}_$(timestamp)")"
app="$(resolve_app)"

cat <<EOF
=== Scenario 2: Chat Open + Scroll Stress (typing + scroll) ===
Goal: capture scroll/typing CPU hotspots.

Setup:
- Ensure a chat has a few hundred messages.

Steps:
- Open chat view.
- Scroll up/down for ~15–30 seconds.
- Tap into the input and type a few words.
- Close the app.
EOF

print_instructions

run_perf_for_scenario "$scenario" "$out_dir" "$app"

"$SCRIPT_DIR/make_graphs.sh" "$out_dir/perf.data" "${FLAMEGRAPH_DIR}"

echo "Artifacts:" 
ls -1 "$out_dir" | sed 's/^/  - /'
