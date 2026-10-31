#!/usr/bin/env bash
# Run a platform-neutral selection of the upstream Lua 5.5 tests in eLua sim.
#
# Each test gets a fresh simulator/Lua process.  Upstream tests may declare
# globals as constants, so running them in one shared Lua state is incorrect.

set -u

ROOT=$(CDPATH= cd -- "$(dirname -- "$0")/.." && pwd)
cd "$ROOT"

ELUA_SIM=${ELUA_SIM:-./elua_lua_sim.elf}
TIMEOUT=${TIMEOUT:-60}

if [ ! -x "$ELUA_SIM" ]; then
  echo "Simulator executable not found: $ELUA_SIM" >&2
  echo "Build it with: lua5.1 build_elua.lua board=sim build_mode=build_dir_linearized build_dir=.build-sim" >&2
  exit 2
fi

# These exercise the VM, parser, garbage collector and standard pure-Lua
# libraries.  Deliberately excluded: tests that require host files, locales,
# dynamic modules, the Lua test C module, or the full upstream test driver.
TESTS=(
  literals
  constructs
  calls
  strings
  utf8
  math
  vararg
  closure
  events
  coroutine
)

failed=0
for test_name in "${TESTS[@]}"; do
  marker="ELUA_LUA55_TEST_PASS ${test_name}"
  output=$(mktemp)
  trap 'rm -f "$output"' EXIT

  printf "lua55 -e \"dofile('src/lua55/testes/%s.lua'); print('%s')\"\nexit\n" \
    "$test_name" "$marker" \
    | timeout "$TIMEOUT" "$ELUA_SIM" >"$output" 2>&1
  run_status=$?

  cat "$output"
  if [ "$run_status" -ne 0 ] || ! grep -Fq "$marker" "$output"; then
    echo "FAIL: ${test_name}" >&2
    failed=1
  else
    echo "PASS: ${test_name}"
  fi

  rm -f "$output"
  trap - EXIT
done

exit "$failed"
