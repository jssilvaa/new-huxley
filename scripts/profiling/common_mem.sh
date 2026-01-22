#!/usr/bin/env bash
set -euo pipefail

# memory profiling helpers

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
  cd "$(dirname "${BASH_SOURCE[0]}")/../.." && pwd
}

# defaults via env
: "${BUILD_DIR:=build/Desktop_Qt_6_10_1-RelWithDebInfo}"
: "${APP_PATH:=}"
: "${OUT_BASE:=profiles/mem}"

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
  local root out_dir
  root="$(repo_root)"
  out_dir="$root/$OUT_BASE/$1"
  mkdir -p "$out_dir"
  echo "$out_dir"
}

write_meta() {
  local out_dir="$1"
  local scenario="$2"
  local tool="$3"
  local app="$4"
  local cmdline="$5"

  {
    echo "scenario=$scenario"
    echo "tool=$tool"
    echo "date=$(date -Is)"
    echo "host=$(hostname)"
    echo "uname=$(uname -a)"
    echo "app=$app"
    echo "cmdline=$cmdline"
    echo "cwd=$(pwd)"
    echo "build_dir=$BUILD_DIR"
  } > "$out_dir/META.txt"
}

print_instructions() {
  cat <<'EOF'
Instructions:
- Perform the scenario steps in the app.
- When finished: close the app window (preferred).
- If you really need to stop early: Ctrl+C in the terminal.
EOF
}
