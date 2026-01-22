#!/usr/bin/env bash
set -euo pipefail

# Common helpers for profiling scripts.
# Usage: source this file from scenario scripts.

require_cmd() {
  command -v "$1" >/dev/null 2>&1 || {
    echo "Missing required command: $1" >&2
    exit 1
  }
}

timestamp() {
  date +"%Y%m%d-%H%M%S"
}

repo_root() {
  # scripts/profiling/common.sh -> repo root is two levels up
  cd "$(dirname "${BASH_SOURCE[0]}")/../.." && pwd
}

# Defaults (override via env)
: "${BUILD_DIR:=build/Desktop_Qt_6_10_1-RelWithDebInfo}"
: "${APP_PATH:=}"
: "${OUT_BASE:=profiles/perf}"
: "${PERF_EVENT:=cpu-clock}"
: "${PERF_FREQ:=199}"
: "${PERF_CALLGRAPH:=dwarf}"
: "${FLAMEGRAPH_DIR:=$HOME/FlameGraph}"

resolve_app() {
  local root
  root="$(repo_root)"

  if [[ -n "${APP_PATH}" ]]; then
    echo "${APP_PATH}"
    return 0
  fi

  if [[ -x "$root/$BUILD_DIR/appchat" ]]; then
    echo "$root/$BUILD_DIR/appchat"
    return 0
  fi

  if [[ -x "$root/build/Desktop_Qt_6_10_1-Debug/appchat" ]]; then
    echo "$root/build/Desktop_Qt_6_10_1-Debug/appchat"
    return 0
  fi

  echo "Could not find app binary. Set BUILD_DIR or APP_PATH." >&2
  echo "Tried: $root/$BUILD_DIR/appchat" >&2
  exit 1
}

ensure_out_dir() {
  local root
  local out_dir
  root="$(repo_root)"
  out_dir="$root/$OUT_BASE/$1"
  mkdir -p "$out_dir"
  echo "$out_dir"
}

write_meta() {
  local out_dir="$1"
  local scenario="$2"
  local app="$3"
  local cmdline="$4"

  {
    echo "scenario=$scenario"
    echo "date=$(date -Is)"
    echo "host=$(hostname)"
    echo "uname=$(uname -a)"
    echo "perf_version=$(perf --version 2>/dev/null || true)"
    echo "app=$app"
    echo "cmdline=$cmdline"
    echo "cwd=$(pwd)"
    echo "build_dir=$BUILD_DIR"
    echo "event=$PERF_EVENT"
    echo "freq=$PERF_FREQ"
    echo "callgraph=$PERF_CALLGRAPH"
  } > "$out_dir/META.txt"
}

print_instructions() {
  cat <<'EOF'
Instructions:
- Perform the scenario steps in the app.
- When finished: close the app window (preferred).
- If you really need to stop early: Ctrl+C in the terminal running perf.
EOF
}

run_perf_for_scenario() {
  local scenario="$1"
  local out_dir="$2"
  local app="$3"

  require_cmd perf

  : "${PERF_STARTUP_DELAY:=2}"     # seconds (override via env)
  : "${PERF_DURATION:=}"           # optional max seconds; empty = until app exits

  echo "Output: $out_dir"
  echo "App:    $app"
  echo "Event:  $PERF_EVENT @ $PERF_FREQ Hz, callgraph=$PERF_CALLGRAPH"

  local perf_data="$out_dir/perf.data"

  # Start app normally (so we don't profile the dynamic loader startup path)
  "$app" &
  local app_pid=$!

  # Give Qt/QML time to finish initialization before we attach perf
  sleep "$PERF_STARTUP_DELAY"

  local cmd=(perf record -e "$PERF_EVENT" -F "$PERF_FREQ" -g --call-graph "$PERF_CALLGRAPH" -o "$perf_data" -p "$app_pid")
  write_meta "$out_dir" "$scenario" "$app" "${cmd[*]}"

  echo "Running: ${cmd[*]}"

  if [[ -n "${PERF_DURATION}" ]]; then
    # Sample for a fixed time window
    "${cmd[@]}" -- sleep "$PERF_DURATION" >/dev/null 2>&1 || true
  else
    # Sample until the app exits
    "${cmd[@]}" >/dev/null 2>&1 &
    local perf_pid=$!

    # Wait for app to exit, then stop perf cleanly
    wait "$app_pid" 2>/dev/null || true
    kill -INT "$perf_pid" 2>/dev/null || true
    wait "$perf_pid" 2>/dev/null || true
  fi

  echo "Captured: $perf_data"
}
