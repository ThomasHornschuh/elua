# eLua Linux Simulator (sim)

This repository contains a host-side eLua target named sim. It is an i386
(32-bit) Linux process, not a RISC-V simulator. It is useful for testing eLua
core functionality, Lua modules and filesystems without FPGA hardware.

The historical simulator expected an i386 Newlib cross compiler. The adapted
target builds with the normal Debian/Ubuntu 32-bit GCC runtime instead.

## Host dependencies

On Debian or Ubuntu install:

~~~sh
sudo apt update
sudo apt install lua5.1 nasm gcc-multilib libc6-dev-i386
~~~

Lua 5.1 is required because the eLua build scripts use Lua 5.1. NASM assembles
the i386 simulator support code; the multilib packages provide the 32-bit
compiler headers and libraries used by gcc -m32.

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

## Lua 5.5.1 alongside eLua Lua 5.1

The image also contains the upstream Lua 5.5.1 interpreter.  Start it from
the eLua shell with `lua55`:

~~~text
eLua# lua55
Starting Lua 5.5 (without eLua modules)
Lua 5.5.1  Copyright (C) 1994-2026 Lua.org, PUC-Rio
> print(_VERSION)
Lua 5.5
>
~~~

`lua` remains the existing eLua Lua 5.1 interpreter.  Lua 5.5.1 is imported
from the official `v5.5.1` source release in `src/lua55/` and is compiled as
a private single translation unit, so both interpreters can reside in one
firmware image.  The first eLua modules, `pd`, `elua` and `cpu`, are available:

~~~text
> print(pd.platform(), pd.cpu(), pd.board())
SIM     LINUX   SIM
~~~

`elua.version()` and `elua.heapstats()` are currently provided.  The former
Lua-5.1-specific emergency-GC controls are intentionally not exposed yet.
`cpu.clock()` returns the configured platform CPU frequency; direct memory and
interrupt operations remain to be ported.

Lua 5.5 `io.open`, `loadfile` and `dofile` also use eLua device-manager paths,
such as `/rom/...` and `/mmc/...`.  The remaining eLua objects and libraries
(for example `pio` and `tmr`) are not registered yet.

### Lua 5.5 regression selection in `sim`

The upstream Lua source includes its comprehensive `src/lua55/testes/` suite.
Many of its tests intentionally require a Unix/DOS filesystem, locale data,
dynamic modules, or the special upstream C test module and therefore do not
belong in a portable eLua smoke test.  The curated runner below executes a
fresh simulator process for each selected platform-neutral test (parser, VM,
garbage collector, coroutines, strings, UTF-8 and math) plus `pd` and virtual
filesystem smoke tests:

~~~sh
bash tests/run_lua55_sim_tests.sh
~~~

The runner prints `PASS: <test>` for every completed test and returns a
non-zero status if a test fails or does not reach its explicit completion
marker.  Set `ELUA_SIM` to test another simulator binary and `TIMEOUT` (in
seconds) to adjust the per-test limit.

## Filesystem images

* `wofs.img` is created automatically in the current directory when WOFS is
  enabled.
* If FAT/MMC support is enabled, an `sdcard.img` in the current directory is
  used as the emulated SD-card image.

## Implementation notes

The target uses the normal Linux C runtime and startup code while retaining
eLua's simulated memory and Linux syscall backend. The build forces non-PIE
i386 objects so eLua's read-only Lua tables retain the memory layout expected
by its Lua 5.1 optimisations.
