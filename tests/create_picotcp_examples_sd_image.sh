#!/usr/bin/env bash
# Create a FAT image with the curated PicoTCP-NG Lua network examples.
#
# The external examples checkout is deliberately an input, not a submodule.
# Its tested revision is recorded in new_doc/PICOTCP_NG.md.

set -euo pipefail

root=$(CDPATH= cd -- "$(dirname -- "$0")/.." && pwd)
examples=${ELUA_EXAMPLES_DIR:-"$root/../elua_examples"}
image=${1:-picotcp-examples-sdcard.img}

if [[ ! -f $examples/net/tcp_echo.lua || ! -f $examples/net/http_get_text.lua ||
      ! -f $examples/romfs_papilio/util.lua || ! -f $examples/modules/debugger.lua ]]; then
  printf 'ELUA_EXAMPLES_DIR must name an elua_examples checkout with net/, romfs_papilio/, and modules/: %s\n' "$examples" >&2
  exit 2
fi
if [[ -e $image ]]; then
  printf 'Refusing to overwrite existing image: %s\n' "$image" >&2
  exit 1
fi
for command in truncate mformat mmd mcopy; do
  command -v "$command" >/dev/null || { printf 'Required command not found: %s\n' "$command" >&2; exit 2; }
done

truncate -s 16M "$image"
mformat -i "$image" ::
mmd -i "$image" ::/net
# Keep original filenames: sim_picotcp enables FatFS long-file-name support.
mcopy -i "$image" "$examples/net/tcp_echo.lua" ::/net/tcp_echo.lua
mcopy -i "$image" "$examples/net/http_get_text.lua" ::/net/http_get_text.lua
mcopy -i "$image" "$examples/romfs_papilio/util.lua" ::/util.lua
mcopy -i "$image" "$examples/modules/debugger.lua" ::/debugger.lua

printf 'Created %s from elua_examples revision %s\n' "$image" \
  "$(git -C "$examples" rev-parse HEAD 2>/dev/null || printf unknown)"
printf 'Run the simulator from this image directory, named sdcard.img, then use /mmc/net/tcp_echo.lua or /mmc/net/http_get_text.lua.\n'
