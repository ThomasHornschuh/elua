#!/usr/bin/env bash
# Create a FAT image containing the curated Lua 5.5 eLua simulator tests.

set -eu

ROOT=$(CDPATH= cd -- "$(dirname -- "$0")/.." && pwd)
cd "$ROOT"

IMAGE=${1:-lua55-tests-sdcard.img}
if [ -e "$IMAGE" ]; then
  echo "Refusing to overwrite existing image: $IMAGE" >&2
  exit 1
fi

for command in truncate mformat mmd mcopy; do
  if ! command -v "$command" >/dev/null 2>&1; then
    echo "Required command not found: $command" >&2
    exit 2
  fi
done

truncate -s 16M "$IMAGE"
# eLua's FatFS configuration supports FAT16, not FAT32.
mformat -i "$IMAGE" ::
mmd -i "$IMAGE" ::/lua55
mmd -i "$IMAGE" ::/lua55/upstream
mmd -i "$IMAGE" ::/lua55/elua
mcopy -i "$IMAGE" tests/lua55/sd/run.lua ::/lua55/run.lua

for test in literals calls strings utf8 math vararg closure events; do
  mcopy -i "$IMAGE" "src/lua55/testes/$test.lua" "::/lua55/upstream/$test.lua"
done
mcopy -i "$IMAGE" src/lua55/testes/constructs.lua ::/lua55/upstream/construct.lua
mcopy -i "$IMAGE" src/lua55/testes/coroutine.lua ::/lua55/upstream/coroutin.lua
for test in pd elua term; do
  mcopy -i "$IMAGE" "tests/lua55/$test.lua" "::/lua55/elua/$test.lua"
done
mcopy -i "$IMAGE" tests/lua55/filesystem.lua ::/lua55/elua/files.lua

echo "Created $IMAGE"
echo "Run it in sim from its directory as sdcard.img."
