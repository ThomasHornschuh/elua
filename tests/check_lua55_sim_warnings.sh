#!/usr/bin/env bash
# Build the native Lua 5.5 simulator and reject every warning diagnostic.

set -eu

ROOT=$(CDPATH= cd -- "$(dirname -- "$0")/.." && pwd)
BUILD_DIR=${1:-.build-sim-lua55-warnings}
LOG=$(mktemp "${TMPDIR:-/tmp}/elua-lua55-sim-build.XXXXXX.log")
trap 'rm -f "$LOG"' EXIT

cd "$ROOT"
if ! lua5.1 build_elua.lua board=sim lua_version=lua55 \
  build_mode=build_dir_linearized build_dir="$BUILD_DIR" >"$LOG" 2>&1; then
  cat "$LOG"
  exit 1
fi

cat "$LOG"
if grep -E '(^|: )warning:' "$LOG"; then
  echo "Lua 5.5 sim build emitted warning diagnostics." >&2
  exit 1
fi
