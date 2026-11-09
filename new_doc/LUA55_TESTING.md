# Lua 5.5 tests on the Linux simulator

This document describes the curated Lua 5.5.1 regression selection for the
eLua `sim` board.  It covers both the automated host runner and the
interactive FAT SD-card image.

## Prerequisites (Debian/Ubuntu)

Install the host compiler, the Lua 5.1 build-script runtime, and mtools for
creating the FAT16 image:

```sh
sudo apt update
sudo apt install build-essential lua5.1 liblua5.1-0-dev luarocks mtools
sudo luarocks install luafilesystem
sudo luarocks install lpack
sudo luarocks install md5
```

`lua5.1` runs `build_elua.lua`; it is not the firmware Lua version. The
`lua_version=lua55` build option selects the Lua 5.5.1 runtime embedded in the
simulator. The `sim` target uses the native host compiler and needs neither
32-bit multilib packages nor NASM.

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
`elua`, `term`, `tmr`, and `cpu` modules plus filesystem access. The timer
smoke test runs directly from `/rom/lua55_timers.lua`. On the native 64-bit
simulator, `cpu.r8/r16/r32` and `cpu.w8/w16/w32` raise an error instead of
attempting unsafe raw-address access; the Lua CPU API only carries 32-bit
addresses and cannot represent native host pointers.

## RPC packet codec regression test

Run the focused native-host codec test with:

```sh
bash tests/run_eluarpc_codec_test.sh
```

It compiles `src/eluarpc.c` with `-Wall -Wextra -Wconversion -Werror` and
checks fixed-width start/end signatures, valid packet decoding, rejection of
corrupted signatures, and rejection of packets that exceed the `u16` wire-size
field. `eluarpc_gen_write` returns `ELUARPC_ERR` for an oversized encoded
packet; callers must provide a buffer large enough for the packet being
encoded.

## Simulator timers

The Linux `sim` board has no hardware timers. `require("tmr")` provides
`tmr.SYS_TIMER`, a `CLOCK_MONOTONIC` microsecond counter with a 1 MHz clock,
and four virtual counters, `tmr.VIRT0` through `tmr.VIRT3`. The system
counter wraps at the platform timer range; use `tmr.gettimediff` or
`tmr.getdiffnow` to calculate elapsed time across a wrap.

Virtual counters are driven by a 10 Hz host timer, including while the
simulator waits for input. Their resolution is 100 ms. Virtual `delay`
converts microseconds to whole ticks by rounding down, and resetting a
virtual timer waits for the next tick. Timer match interrupts are not
supported. Both system and virtual delays busy-wait.

Host scheduling may delay execution and coalesce periodic signals under
load, so virtual counters can lose ticks and neither timer guarantees
real-time latency. `CLOCK_MONOTONIC` is unaffected by wall-clock changes
but does not count Linux suspend time. The smoke test allows up to two
seconds of additional scheduling latency rather than asserting exact timing;
this is a test tolerance, not a real-time guarantee.

## Optional PicoTCP-NG TAP profile

The default `board=sim` build and regression runner remain network-free. For
the opt-in native Linux profile, pinned source build, TAP setup and cleanup,
GPL distribution warning, supported API surface, and privileged TCP test, see
[PicoTCP-NG on the Linux simulator](PICOTCP_NG.md). Building the opt-in profile
does not create or attach to a TAP interface.

## GitHub Actions

The [Lua 5.5 Linux simulator workflow](../.github/workflows/lua55-simulator.yml)
runs on every push and pull request, and can also be started manually. It
installs the native build tools, Lua 5.1 headers and interpreter, and the
LuaRocks modules used by the build script. The workflow verifies those modules
are available to `lua5.1`, builds into `.build-ci-sim-lua55`, and runs the same
curated test script against that simulator. Build and test logs are uploaded
as an artifact if either step fails.

The local-equivalent build and test commands are the ones shown above; CI uses
a separate build directory to keep its generated files isolated. To build with
a strict warning check locally, run:

```sh
bash tests/check_lua55_sim_warnings.sh
```

The check rejects compiler, preprocessor, and linker warning diagnostics while
allowing informational `#pragma message` output.

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
