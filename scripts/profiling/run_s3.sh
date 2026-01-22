#!/usr/bin/env bash
set -euo pipefail

# scenario 3 send burst

SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
# shellcheck source=common.sh
source "$SCRIPT_DIR/common.sh"

scenario="s3_send_burst"
out_dir="$(ensure_out_dir "${scenario}_$(timestamp)")"
app="$(resolve_app)"

cat <<EOF
=== Scenario 3: Send Message Burst ===
Goal: capture send/update CPU hotspots.

Steps:
- Open an active chat.
- Send ~20 short messages quickly.
- Send ~5 longer messages.
- Close the app.
EOF

print_instructions

run_perf_for_scenario "$scenario" "$out_dir" "$app"

"$SCRIPT_DIR/make_graphs.sh" "$out_dir/perf.data" "${FLAMEGRAPH_DIR}"

echo "Artifacts:" 
ls -1 "$out_dir" | sed 's/^/  - /'
