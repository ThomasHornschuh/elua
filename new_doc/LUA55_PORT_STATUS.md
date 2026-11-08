# eLua Lua 5.5 port: handover status

This is the technical handover document for the Lua 5.5 work on branch
`lua_5_5_port`.  It is intended for a follow-up developer or coding agent.
It records the current architecture, deliberate constraints, supported build
paths, and the work that remains.  It is not a promise that every historical
eLua board or module has been validated with Lua 5.5.

## Current scope

* Upstream **Lua 5.5.1** is imported in `src/lua55/`.
* Firmware selects exactly one embedded Lua runtime at build time:
  `lua_version=lua51` (the default) or `lua_version=lua55`.
* The original eLua module sources in `src/modules/` are shared by both
  runtimes.  There is intentionally no `modules55/` tree.
* The Linux `sim` target is the primary regression target and has a curated
  Lua 5.5 test selection, including an interactive FAT SD-card image.
* `riscv32_spike` and the minimal Bonfire Arty profile are configured for the
  Picolibc-equipped RISC-V toolchain.  The Spike build has been exercised;
  the Arty profile has been compile-checked but not hardware-tested.

## Non-negotiable design decisions

### One Lua runtime per firmware image

Do not build Lua 5.1 and Lua 5.5 into the same image and do not reintroduce
an `onelua.c`-based coexistence design.  `build_elua.lua` chooses either
`src/lua/` or `src/lua55/`; Lua 5.5 is compiled from its ordinary individual
upstream source files.  This avoids symbol, allocator, global-state, and
shell-command ambiguity.

The host interpreter which runs `build_elua.lua` remains Lua 5.1.  It is
unrelated to the runtime embedded in the resulting firmware.

```sh
# Historic/default firmware runtime
lua5.1 build_elua.lua board=sim lua_version=lua51

# Firmware containing Lua 5.5.1
lua5.1 build_elua.lua board=sim lua_version=lua55
```

### No Lua 5.5 ROM tables or LTR optimisation

eLua's Lua-5.1 ROM-table/LTR implementation depends on private Lua VM data
structures.  It is not safe to carry that implementation forward through
macro aliases.  Lua 5.5 builds therefore set `LUA_OPTIMIZE_MEMORY=0` and use
ordinary, mutable RAM-backed Lua tables.

Consequences:

* `lua_isrotable()` deliberately evaluates to false on the Lua 5.5 path.
* Code relying on immutable ROM tables, `TValue`, `Table`, `lua_State`, or
  other Lua-5.1 internals needs a real redesign using public Lua APIs.
* The Lua-5.1-specific eLua emergency-GC controls (for example
  `elua.egc_setup`) are not exposed by Lua 5.5.

Do not attempt to restore these optimisations as a small compatibility patch.
It would be a separate Lua-5.5 VM-integration project and must have its own
design and test plan.

### Reuse original module sources

Existing map-based eLua modules should continue to live in `src/modules/`.
`inc/lua_compat.h`, included through the existing LTR headers where relevant,
bridges the small legacy API surface currently needed by shared modules:

* `luaL_reg` / `luaL_register`
* `lua_objlen`
* `luaL_typerror`
* `luaL_putchar`
* `lua_isrotable` (always false for Lua 5.5)

When a module needs different behaviour, add the smallest documented
`#if defined(ELUA_LUA55)` branch around that behaviour.  Prefer a public Lua
5.5 API to compatibility aliases.  Do not fork a whole module merely because
it used an LTR declaration.  See [the module porting guide](LUA55_MODULE_PORTING.md)
for the `pd` example and the required porting checklist.

## Source map

| Path | Role |
| --- | --- |
| `build_elua.lua` | Defines `lua_version`, selects exactly one Lua source tree, defines `ELUA_LUA55`, disables Lua 5.5 memory optimisation, and excludes `onelua.c` and upstream test sources from firmware builds. |
| `src/lua/` | Existing eLua-customised Lua 5.1 runtime; default build selection. |
| `src/lua55/` | Imported upstream Lua 5.5.1 source plus eLua integration changes. |
| `src/lua55/linit.c` | Opens standard Lua libraries, selected eLua modules, and the platform module namespace for Lua 5.5. |
| `inc/lua_compat.h` | Narrow Lua-5.1-to-5.5 compatibility layer for shared eLua modules. |
| `src/modules/` | Shared eLua C modules.  Whether a module is visible at runtime still depends on board configuration. |
| `boards/known/sim.lua` | Primary host regression configuration. |
| `boards/known/bonfire_arty_minimal.lua` | Minimal, no-network Bonfire Arty Lua 5.5-oriented profile. |
| `src/platform/riscv32spike/conf.lua` | Spike/Picolibc RISC-V compiler and linker settings. |
| `src/platform/bonfire/conf.lua` | Bonfire build settings, including the Picolibc minimal-profile path. |
| `tests/run_lua55_sim_tests.sh` | Isolated automated simulator regression runner. |
| `tests/create_lua55_sd_image.sh` | Reproducibly creates the interactive Lua 5.5 FAT test image. |

## Module registration details

Lua 5.5 uses the usual `luaL_Reg`/`luaL_setfuncs` registration path.  In
`src/lua55/linit.c`, `LUA_PLATFORM_LIBS_ROM` registers selected generic eLua
modules and `PLATFORM_MODULES_LIBS_ROM` constructs the historical platform
namespace.  For example, Bonfire platform modules remain available below
`bonfire`, such as `bonfire.riscv` and `bonfire.gdbserver`.

When adding a module, distinguish these two requirements:

1. Its C source must compile, normally from `src/modules/`.
2. Its open function must be selected by the board configuration and emitted
   into the generated module-registration macros.

A successful compile alone does not prove that `require` or the expected
global module name is available in Lua.  Keep `config/modules.lua`,
`src/modules/auxmods.h`, and the relevant `boards/known/*.lua` selection in
sync for new modules.

## Target status and build recipes

### Linux `sim`

`sim` is a 32-bit Linux eLua process, not a RISC-V simulator.  It uses the
host glibc runtime with normal `gcc -m32`; it does **not** use Picolibc or an
old i386 Newlib cross compiler.  The board selects `pd`, all standard Lua
libraries, `term`, and `elua`, with line editing/history enabled for both the
eLua shell and Lua REPL.

On Debian/Ubuntu:

```sh
sudo apt update
sudo apt install lua5.1 nasm gcc-multilib libc6-dev-i386
lua5.1 build_elua.lua board=sim lua_version=lua55 \
  build_mode=build_dir_linearized build_dir=.build-sim-lua55
./run_elua_sim.sh
```

The simulator's filesystem bridge lets Lua 5.5 `io.open`, `loadfile`, and
`dofile` address eLua paths such as `/rom/...` and `/mmc/...`.  Linenoise uses
raw terminal mode only for an interactive TTY; redirected test input must not
enable raw mode.  Ctrl-D leaves the Lua REPL and returns to the eLua shell.

See [the simulator README](README.md) for everyday use and
[Lua 5.5 testing](LUA55_TESTING.md) for the full test workflow.

### RISC-V Spike

`riscv32_spike` is the software RISC-V target using the proxy kernel.  Its
Lua 5.5 build uses the cross compiler's Picolibc specs and defines
`ELUA_SHELL_EOF_CTRL_D`, so Ctrl-D is used instead of the historical Ctrl-Z
Lua exit sequence.

```sh
lua5.1 build_elua.lua board=riscv32_spike lua_version=lua55 \
  build_mode=build_dir_linearized build_dir=.build-spike-lua55
spike --isa=rv32im_zicsr_zifencei -m256 pk ./elua_lua_riscv32_spike.elf
```

The exact locations of `spike` and the RV32 proxy kernel (`pk`) are local
toolchain choices; put them on `PATH` or use explicit local paths.  Do not
put such paths in eLua configuration files.

### Minimal Bonfire Arty

`bonfire_arty_minimal` is intentionally a small, no-network profile:

```sh
lua5.1 build_elua.lua board=bonfire_arty_minimal lua_version=lua55
```

It retains MMC/SD and FatFS, serial console, shell/linenoise, `pd`, standard
Lua libraries, `term`, `elua`, `uart`, and `tmr`.  It also selects the
Bonfire-specific `riscv` and `gdbserver` Lua modules; the GDB remote stub uses
UART1 at 500000 baud.  It deliberately omits `net`, PicoTCP, and the Ethernet
Lite driver.  This profile has been compiler/linker checked but still needs a
real-board smoke test for console, SD/FatFS, timers, UART, and the GDB stub.

The profile also carries modern RISC-V fixes required by current GCC:
`rv32im_zicsr_zifencei`, the current CSR naming in startup code, and an eLua
linker configuration suitable for Picolibc rather than `libgloss`.

## Picolibc policy

The RISC-V Picolibc builds must rely on the selected cross GCC's normal
installation discovery:

```text
-specs=picolibc.specs
```

The specs file supplies the appropriate headers, libraries, and linker setup.
Therefore **do not** add an absolute sysroot, a Picolibc archive path, or a
user-specific environment-variable requirement to `conf.lua`.  A correctly
installed cross compiler should build with its own defaults plus the specs
option.  eLua continues to provide its platform startup and syscall stubs
where required.

If a Picolibc build fails, first inspect the selected compiler and whether it
actually ships `picolibc.specs`; do not paper over the failure by hard-coding
the layout of one machine's distribution packages.

## Tests and expected verification flow

The imported upstream suite remains in `src/lua55/testes/`, but many upstream
tests assume a Unix/DOS filesystem, locales, dynamic C modules, or Lua's
special test C module.  The eLua suite is deliberately curated rather than a
blind invocation of all upstream tests.

For the simulator, build Lua 5.5 and run:

```sh
bash tests/run_lua55_sim_tests.sh
```

It starts a fresh simulator process for every test, prints that test's full
output, requires an explicit completion marker, and returns non-zero on a
failure.  The selection covers Lua parser/VM/library behaviour plus eLua
module and virtual-filesystem smoke tests.  The current curated selection can
also be used interactively from a FAT16 image:

```sh
bash tests/create_lua55_sd_image.sh
# Copy lua55-tests-sdcard.img as sdcard.img into the simulator's CWD.
# Then, at eLua#:
lua /mmc/lua55/run.lua list
lua /mmc/lua55/run.lua all
```

`mtools` is needed only for image generation; see
[LUA55_TESTING.md](LUA55_TESTING.md) for the full apt package list and
workflow.

For every future module or runtime change, at minimum build both runtime
selections for `sim`, run the Lua 5.5 regression script, and add a focused
smoke test if the Lua-visible behaviour changed.  Hardware-related modules
also need validation on their actual board.

## Known gaps and safe next steps

1. **Port modules incrementally.**  Inventory all selected eLua modules for
   private Lua-5.1 API use.  Keep portable code shared and give each actual
   incompatibility a narrow, tested Lua 5.5 branch.
2. **Expand tests by capability.**  Add tests only when their prerequisites
   can be provided on eLua.  Do not make a Linux-only upstream test a
   mandatory firmware regression without adapting its assumptions.
3. **Validate RISC-V targets.**  Rebuild Spike after runtime/module changes
   and test the Bonfire minimal image on real hardware before claiming board
   support.
4. **Treat ROM tables and emergency GC as separate projects.**  They are
   explicitly out of scope for the present port, not accidental omissions.
5. **Preserve Lua 5.1.**  It remains the default and must continue to build
   while the migration proceeds.

Before making structural changes, read this document together with
`LUA55_MODULE_PORTING.md` and `LUA55_TESTING.md`, inspect the current board
configuration, and verify that a proposed change does not violate the
one-runtime-per-image and no-private-VM-internals rules.
