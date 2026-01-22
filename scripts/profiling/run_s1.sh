#!/usr/bin/env bash
set -euo pipefail

# Scenario 1 — Cold Start to Usable UI

SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
# shellcheck source=common.sh
source "$SCRIPT_DIR/common.sh"

scenario="s1_coldstart"
out_dir="$(ensure_out_dir "${scenario}_$(timestamp)")"
app="$(resolve_app)"

cat <<EOF
=== Scenario 1: Cold Start to Usable UI ===
Goal: capture startup CPU hotspots.

Steps:
- Start the app.
- Wait until first screen is usable.
- Optional: perform a login to reach first interactive state.
- Close the app.
EOF

print_instructions

run_perf_for_scenario "$scenario" "$out_dir" "$app"

"$SCRIPT_DIR/make_graphs.sh" "$out_dir/perf.data" "${FLAMEGRAPH_DIR}"

echo "Artifacts:" 
ls -1 "$out_dir" | sed 's/^/  - /'
