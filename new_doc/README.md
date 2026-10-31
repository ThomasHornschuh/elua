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
