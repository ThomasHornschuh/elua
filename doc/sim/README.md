# Overview

The eLua `sim` target runs as a native Linux process. It does not emulate CPU
instructions or hardware peripherals, but it is useful for testing eLua core
behaviour and modules without hardware dependencies.

Its features include:

* A Linux-console implementation of the eLua terminal.
* Configurable simulated RAM.
* Emulated WOFS and optional SD-card images.
* Full ROM filesystem support.
* Native builds for the host architecture; no i386 cross compiler, NASM, or
  32-bit x86 multilib is required.

## Environment

The simulator requires a Linux host with GCC and normal C development headers.
It has been validated on RV64 Linux and is designed to build natively on
x86-64 and AArch64 Linux where a matching host compiler is available.

On Debian or Ubuntu, install the native build tools and the Lua 5.1 runtime
used by the build script:

```
sudo apt install build-essential lua5.1
```

`lua5.1` runs `build_elua.lua`; it is not the Lua runtime embedded in the
firmware image.

## How to build

Build the default Lua 5.1 image with the native host compiler:

```
lua5.1 build_elua.lua board=sim
```

To build Lua 5.5.1 instead:

```
lua5.1 build_elua.lua board=sim lua_version=lua55 \
  build_mode=build_dir_linearized build_dir=.build-sim-lua55
```

To set the simulated RAM size, pass `-DSIM_MEM_SIZE`:

```
lua5.1 build_elua.lua board=sim -DSIM_MEM_SIZE=64*1024
```

The default `SIM_MEM_SIZE` is defined in
[`cpu_linux.h`](../../src/platform/sim/cpu_linux.h). The builder creates
`elua_lua_sim.elf` (or a target-specific variant) in the output directory.

## How to use

Run the simulator using `run_elua_sim.sh`, which configures the console and
starts the executable:

```
./run_elua_sim.sh
```

After startup, the regular eLua shell is available. Run `lua` to enter the Lua
interpreter. Use Ctrl-D to leave Lua and `exit` at the eLua prompt to stop the
simulator.

Hardware-specific facilities such as GPIO, UART, and interrupt configuration
are not emulated.

## WOFS emulator

When WOFS is enabled, it is stored in `wofs.img` in the simulator's current
working directory. The file is created automatically when missing.

## SD card emulator

When FAT filesystem support is enabled, the simulator uses `sdcard.img` in its
current working directory as the emulated SD card. An image can be created
from a FAT-formatted SD card, for example:

```
dd if=/dev/sdX of=sdcard.img bs=4M
```

Replace `/dev/sdX` with the correct block device. The Lua 5.5 test-image
workflow is documented in [`new_doc/LUA55_TESTING.md`](../../new_doc/LUA55_TESTING.md).
