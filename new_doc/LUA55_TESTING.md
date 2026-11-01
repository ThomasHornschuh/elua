# Lua 5.5 tests on the Linux simulator

This document describes the curated Lua 5.5.1 regression selection for the
eLua `sim` board.  It covers both the automated host runner and the
interactive FAT SD-card image.

## Prerequisites (Debian/Ubuntu)

Install the host compiler, the Lua 5.1 build-script runtime, and mtools for
creating the FAT16 image:

```sh
sudo apt update
sudo apt install build-essential gcc-multilib libc6-dev-i386 nasm \
  lua5.1 liblua5.1-0-dev luarocks mtools
sudo luarocks install luafilesystem
sudo luarocks install lpack
sudo luarocks install md5
```

`lua5.1` runs `build_elua.lua`; it is not the firmware Lua version.  The
`lua_version=lua55` build option selects the Lua 5.5.1 runtime embedded in the
simulator.

`mtools` provides `mformat`, `mmd`, and `mcopy`, which are used by the SD image
generator.  No loop device, mount operation, or root access is needed to
create the image.

## Build Lua 5.5 for `sim`

From the repository root:

```sh
lua5.1 build_elua.lua board=sim lua_version=lua55 \
  build_mode=build_dir_linearized build_dir=.build-sim-lua55
```

This creates `elua_lua_sim.elf`.

## Automated regression runner

Run:

```sh
bash tests/run_lua55_sim_tests.sh
```

The runner prints the complete Lua and simulator output for every test, then
prints `PASS: <test>` or `FAIL: <test>`.  It starts a fresh simulator process
per test, which is the authoritative regression mode because upstream tests
can modify globals.

The selected upstream tests are parser/VM/library tests (`literals`,
`constructs`, `calls`, `strings`, `utf8`, `math`, `vararg`, `closure`,
`events`, `coroutine`).  eLua smoke tests cover the board-configured `pd`,
`elua`, and `term` modules plus filesystem access.

## Interactive FAT SD-card image

The generator is versioned in this repository:

```sh
tests/create_lua55_sd_image.sh
```

It creates the ignored, generated file `lua55-tests-sdcard.img` (16 MiB,
FAT16).  It contains a launcher and all selected test scripts below
`/mmc/lua55/`.  FAT16 and 8.3 filenames are intentional: they work with the
eLua FatFS configuration used by `sim`.

The simulator opens an image named `sdcard.img` from its *current working
directory*.  Keep an existing example image intact by using a temporary
directory:

```sh
mkdir -p /tmp/elua-lua55-tests
cp lua55-tests-sdcard.img /tmp/elua-lua55-tests/sdcard.img
cd /tmp/elua-lua55-tests
/absolute/path/to/elua/elua_lua_sim.elf
```

At `eLua#`:

```text
lua /mmc/lua55/run.lua list
lua /mmc/lua55/run.lua literals
lua /mmc/lua55/run.lua all
```

`all` has been verified for the current curated selection.  For CI-style
isolation, prefer `tests/run_lua55_sim_tests.sh`.

## Files in the repository

| Path | Purpose |
| --- | --- |
| `tests/create_lua55_sd_image.sh` | Reproducibly creates the FAT16 image. |
| `tests/lua55/sd/run.lua` | SD-card launcher and test selection. |
| `tests/lua55/` | eLua-specific module/filesystem smoke tests. |
| `src/lua55/testes/` | Imported upstream Lua 5.5 test sources. |
