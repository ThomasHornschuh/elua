# eLua Linux Simulator (sim)

This repository contains a host-side eLua target named `sim`. It is a native
Linux process, not a RISC-V instruction simulator. It is useful for testing
eLua core functionality, Lua modules and filesystems without FPGA hardware.

The simulator builds with the host GCC and Linux C runtime. It does not require
an i386 cross compiler, 32-bit multilib packages, or NASM, and therefore builds
natively on RV64 Linux as well as on normal x86-64 and AArch64 Linux hosts.

## Host dependencies

On Debian or Ubuntu install:

~~~sh
sudo apt update
sudo apt install build-essential lua5.1
~~~

Lua 5.1 is required because the eLua build scripts use Lua 5.1; it is not the
Lua version embedded in the simulator. The native host compiler and its normal
Linux development headers are sufficient.

## Build

Run the following from the repository root:

~~~sh
lua5.1 build_elua.lua board=sim build_mode=build_dir_linearized build_dir=.build-sim
~~~

The generated executable is `elua_lua_sim.elf`. Build output is placed below
`.build-sim/`; both it and the executable are ignored by Git.

## Run

~~~sh
./run_elua_sim.sh
~~~

The eLua shell appears as:

~~~text
eLua#
~~~

For example:

~~~text
eLua# lua
> print(pd.platform())
SIM
>
~~~

Press **Ctrl-D** to leave the Lua interpreter and return to the eLua shell.
At the eLua shell, use `exit` to stop the simulator.

## Selecting the Lua runtime

An eLua image contains exactly one Lua interpreter.  The build-script option
`lua_version` selects it; it does **not** select the host Lua which executes
`build_elua.lua`.  `lua51` is the default and preserves the historic eLua
build.  Select Lua 5.5.1 explicitly:

~~~sh
lua5.1 build_elua.lua board=sim lua_version=lua55 \
  build_mode=build_dir_linearized build_dir=.build-sim-lua55
~~~

Start the selected interpreter from the eLua shell with the regular `lua`
command:

~~~text
eLua# lua
Lua 5.5.1  Copyright (C) 1994-2026 Lua.org, PUC-Rio
> print(_VERSION)
Lua 5.5
>
~~~

Lua 5.5.1 is imported from the official `v5.5.1` source release in
`src/lua55/` and is compiled from its normal individual source files; the
upstream convenience file `onelua.c` is not part of an eLua build.  Lua 5.5
uses the normal eLua sources in `src/modules/`; the board configuration selects
which modules are registered.  For `sim`, these include `pd`, `elua` and
`term`:

~~~text
> print(pd.platform(), pd.cpu(), pd.board())
SIM     LINUX   SIM
~~~

`elua.version()` and `elua.heapstats()` are available.  The
Lua-5.1-specific emergency-GC controls are intentionally omitted.  The
Lua-5.1 ROM-table/LTR implementation relies on VM internals, so Lua 5.5 uses
ordinary RAM-backed module tables for now.

Lua 5.5 `io.open`, `loadfile` and `dofile` also use eLua device-manager paths,
such as `/rom/...` and `/mmc/...`.

### Lua 5.5 regression selection in `sim`

For the complete setup, automated test flow, and interactive SD-card workflow,
see [Lua 5.5 testing](LUA55_TESTING.md).

For guidance on making an eLua C module build with both Lua 5.1 and Lua 5.5,
including a `pd` walkthrough and a coding-agent checklist, see
[Lua 5.5 module porting](LUA55_MODULE_PORTING.md).

For the Lua 5.5 architecture, target status, Picolibc policy, deliberate
compatibility boundaries, and follow-up-agent handover, see
[Lua 5.5 port status](LUA55_PORT_STATUS.md).  For RISC-V Newlib versus
Picolibc toolchain requirements and the planned automatic selection, see
[RISC-V toolchains](RISCV_TOOLCHAINS.md).

The upstream Lua source includes its comprehensive `src/lua55/testes/` suite.
Many of its tests intentionally require a Unix/DOS filesystem, locale data,
dynamic modules, or the special upstream C test module and therefore do not
belong in a portable eLua smoke test.  The curated runner below executes a
fresh simulator process for each selected platform-neutral test (parser, VM,
garbage collector, coroutines, strings, UTF-8 and math) plus configured eLua
module and virtual-filesystem smoke tests:

~~~sh
bash tests/run_lua55_sim_tests.sh
~~~

The runner prints `PASS: <test>` for every completed test and returns a
non-zero status if a test fails or does not reach its explicit completion
marker.  Set `ELUA_SIM` to test another simulator binary and `TIMEOUT` (in
seconds) to adjust the per-test limit.

### Interactive tests from an emulated SD card

Create a separate FAT image containing the same curated test selection:

~~~sh
bash tests/create_lua55_sd_image.sh
~~~

The simulator opens `sdcard.img` in its current directory.  To preserve an
existing image, start it in a temporary directory containing a copy of the
test image:

~~~sh
mkdir -p /tmp/elua-lua55-tests
cp lua55-tests-sdcard.img /tmp/elua-lua55-tests/sdcard.img
( cd /tmp/elua-lua55-tests && /path/to/elua_lua_sim.elf )
~~~

At the eLua prompt, list tests or run one/all of them:

~~~text
eLua# lua /mmc/lua55/run.lua list
eLua# lua /mmc/lua55/run.lua literals
eLua# lua /mmc/lua55/run.lua all
~~~

## Filesystem images

* `wofs.img` is created automatically in the current directory when WOFS is
  enabled.
* If FAT/MMC support is enabled, an `sdcard.img` in the current directory is
  used as the emulated SD-card image.

## Implementation notes

The target uses the normal Linux C runtime and startup code while retaining
eLua's simulated memory and Linux host backend. The backend uses syscall
numbers supplied by the host Linux headers, not architecture-specific inline
assembly. The build retains non-PIE flags for compatibility with the historic
Lua 5.1 read-only-table layout; it does not force a host CPU architecture or
32-bit ABI.
