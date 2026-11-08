#!/usr/bin/env bash
# Build and run native-host regression coverage for the eLua RPC packet codec.

set -eu

ROOT=$(CDPATH= cd -- "$(dirname -- "$0")/.." && pwd)
BUILD_DIR=$(mktemp -d "${TMPDIR:-/tmp}/elua-eluarpc-test.XXXXXX")
trap 'rm -rf "$BUILD_DIR"' EXIT

${CC:-gcc} -std=c99 -Wall -Wextra -Wconversion -Werror \
  -I"$ROOT/inc" -I"$ROOT/src/platform/sim" \
  "$ROOT/tests/eluarpc_codec_test.c" "$ROOT/src/eluarpc.c" \
  -o "$BUILD_DIR/eluarpc_codec_test"
"$BUILD_DIR/eluarpc_codec_test"
