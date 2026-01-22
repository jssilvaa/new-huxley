#!/usr/bin/env bash
set -euo pipefail

# Valgrind Memcheck — Scenario 2 (scroll/typing)

SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
# shellcheck source=common_mem.sh
source "$SCRIPT_DIR/common_mem.sh"

require_cmd valgrind

scenario="vg_s2_scroll_typing"
out_dir="$(ensure_out_dir "${scenario}_$(timestamp)")"
app="$(resolve_app)"

# Noise reduction defaults (override via env)
# - VG_CLEAN_MODE=1: bias toward fewer Wayland/GTK/fontconfig paths so Memcheck results
#   more often point into your code.
: "${VG_CLEAN_MODE:=1}"
: "${VG_REPO_GREP:=1}"
: "${VG_CONTEXT_LINES:=2}"

if [[ "$VG_CLEAN_MODE" == "1" ]]; then
  # Force X11 backend to avoid many Wayland/EGL integration paths.
  : "${QT_QPA_PLATFORM:=xcb}"
fi

: "${VG_SUPP:=$SCRIPT_DIR/qt.supp}"
: "${VG_GEN_SUPP:=0}"

cat <<EOF
=== Valgrind: Scenario 2 (Scroll + Typing) ===
Goal: find definite leaks + invalid accesses in chat UI.

Setup:
- Ensure a chat has a few hundred messages.

Steps:
- Start the app.
- Open chat view.
- Scroll up/down for ~10–20 seconds.
- Type a few words.
- Close the app.

Note: Valgrind is slow. Keep the run short.
EOF

print_instructions

if [[ "$VG_CLEAN_MODE" == "1" ]]; then
  echo
  echo "Clean mode: QT_QPA_PLATFORM=${QT_QPA_PLATFORM:-}" 
  if [[ ! -f "$VG_SUPP" ]]; then
    echo "Note: suppression file not found at $VG_SUPP" >&2
    echo "      You can generate one with: VG_GEN_SUPP=1 scripts/profiling/run_vg_s2.sh" >&2
  fi
fi

# Optional: generate suppression file if you want (one-time).
if [[ "$VG_GEN_SUPP" == "1" ]]; then
  echo "Generating suppression file: $VG_SUPP"
  # First run will create suppressions in stderr; we tee and extract blocks.
  valgrind --tool=memcheck --gen-suppressions=all --error-limit=no \
    --leak-check=full --show-leak-kinds=definite,indirect \
    --track-origins=yes --num-callers=30 \
    --log-file="$out_dir/valgrind.raw.log" \
    "$app" || true
  # Extract suppressions blocks (simple heuristic)
  awk '/^{/{flag=1} flag{print} /^}$/ {flag=0; print ""}' "$out_dir/valgrind.raw.log" > "$VG_SUPP" || true
  echo "Wrote: $VG_SUPP"
  exit 0
fi

LOG="$out_dir/valgrind.log"
SUM="$out_dir/valgrind.summary.txt"
APP_HITS="$out_dir/valgrind.app_hits.txt"

cmd=(
  valgrind --tool=memcheck
  --leak-check=full
  --show-leak-kinds=definite,indirect
  --errors-for-leak-kinds=definite,indirect
  --track-origins=yes
  --num-callers=30
  --error-limit=no
  --verbose
  --log-file="$LOG"
  --fullpath-after="$(repo_root)/"
  --read-var-info=yes
  --keep-debuginfo=yes
)


# Use suppressions if present (Qt/GLib noise reduction)
if [[ -f "$VG_SUPP" ]]; then
  cmd+=( --suppressions="$VG_SUPP" )
fi

write_meta "$out_dir" "$scenario" "valgrind-memcheck" "$app" "${cmd[*]} -- $app"

echo "Output: $out_dir"
echo "App:    $app"
echo "Running: ${cmd[*]} -- $app"

# Run (Valgrind stops when app exits)
"${cmd[@]}" -- "$app" || true

# Extract lines that touch this repo (fast signal for "is this our bug?")
if [[ "$VG_REPO_GREP" == "1" ]]; then
  root="$(repo_root)"
  if command -v grep >/dev/null 2>&1; then
    # Extract only error contexts that touch appchat binary symbols OR repo paths
    {
    echo "== App-owned contexts (heuristic) =="

    # 1) Any stack that mentions your binary name directly
    awk '
        BEGIN{in=0}
        /^==[0-9]+==/ {line=$0}
        /==[0-9]+== (Invalid|Conditional|Use|Jump|Syscall|Thread|Process)/ {in=1; buf=line "\n"; next}
        in {buf=buf $0 "\n"}
        /==[0-9]+==$/ {next}
        in && /==[0-9]+==$/ {next}
        in && /^==[0-9]+==\s*$/ {next}
        in && /==[0-9]+==$/ {next}
        in && /^==[0-9]+==\s*$/ {next}
        in && /^==[0-9]+==\s*$/ {next}
        in && /==[0-9]+==\s*$/ {next}
        in && /==[0-9]+==\s*$/ {next}
        # end of a context is typically a blank '==pid==' line OR a summary section
        in && (/^==[0-9]+==\s*$/ || /ERROR SUMMARY:/ || /LEAK SUMMARY:/) {
        if (buf ~ /(appchat)/) print buf "\n---\n"
        in=0; buf=""
        }
    ' "$LOG"

    # 2) Any stack that contains your repo path (if it exists)
    root="$(repo_root)"
    if grep -qF "$root/" "$LOG"; then
        echo
        echo "== Contexts with repo paths =="
        grep -n -C 2 "$root/" "$LOG" || true
    fi
    } > "$APP_HITS" || true
  fi
fi

# Produce a short summary (works even if log format changes a bit)
{
  echo "== Valgrind summary =="
  echo "log=$LOG"
  if [[ "$VG_CLEAN_MODE" == "1" ]]; then
    echo "clean_mode=1"
    echo "QT_QPA_PLATFORM=${QT_QPA_PLATFORM:-}"
  fi
  echo
  echo "-- Error summary lines --"
  grep -E "ERROR SUMMARY|LEAK SUMMARY|definitely lost|indirectly lost|possibly lost|still reachable" "$LOG" || true
  echo
  echo "-- App-related hits (repo path; see valgrind.app_hits.txt) --"
  if [[ -s "$APP_HITS" ]]; then
    echo "hits_file=$APP_HITS"
    # show just a snippet in the summary
    head -n 80 "$APP_HITS" || true
  else
    echo "(none)"
  fi
  echo
  echo "-- Top contexts (first 30) --"
  grep -nE "at 0x|by 0x|==[0-9]+==" "$LOG" | head -n 120 || true
} > "$SUM"

echo "Done."
echo "Artifacts:"
ls -1 "$out_dir" | sed 's/^/  - /'
