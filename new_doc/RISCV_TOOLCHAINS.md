# RISC-V toolchains: Newlib and Picolibc

This document describes the C-library variants used by eLua's RISC-V targets.
It is intentionally separate from the Lua runtime choice: `lua_version=lua51`
or `lua_version=lua55` selects the Lua interpreter embedded in firmware; it
does **not** select Newlib or Picolibc.

## Why both variants matter

Two valid RISC-V cross-toolchain layouts are in use:

| Variant | Typical installation | C library discovery |
| --- | --- | --- |
| Newlib | A locally built `riscv-gnu-toolchain`, for example `/home/thomas/opt/riscv-gnu-toolchain/bin/riscv64-unknown-elf-gcc` | GCC's normal multilib/sysroot lookup, including `libc.a` |
| Picolibc | Debian's RISC-V embedded GCC together with a separately installed Picolibc package, or a GCC built with Picolibc | GCC finds `picolibc.specs`; the build adds `-specs=picolibc.specs` |

A compiler named `riscv64-unknown-elf-gcc` can still generate the required
32-bit firmware when it supports the appropriate multilib.  The local Newlib
toolchain has been checked with:

```sh
/home/thomas/opt/riscv-gnu-toolchain/bin/riscv64-unknown-elf-gcc \
  -march=rv32im_zicsr_zifencei -mabi=ilp32 -print-file-name=libc.a
```

It returns its `rv32im/ilp32/libc.a`.  It does not provide Picolibc merely
because its target triplet is RISC-V.

## Target requirements

`bonfire_arty_minimal` requires a compiler capable of:

```text
-march=rv32im_zicsr_zifencei -mabi=ilp32
```

The current board configuration in `src/platform/bonfire/conf.lua` always
selects Picolibc for this board.  It adds `ELUA_PICOLIBC` and
`-specs=picolibc.specs`.  Consequently, the current branch builds this board
only with a Picolibc-equipped toolchain.  A Newlib toolchain can generate the
machine code, but the current configuration does not yet select its linker and
C-library path.

`riscv32_spike` currently also selects Picolibc unconditionally.

## Prefix discovery

`build_data.lua` discovers a RISC-V compiler in this order:

1. `$TARGET_PREFIX-gcc`;
2. `riscv64-unknown-elf-gcc`;
3. `riscv32-unknown-elf-gcc`;
4. `riscv-none-embed-gcc`.

Set `TARGET_PREFIX` without a trailing hyphen.  For the local toolchain:

```sh
export PATH=/home/thomas/opt/riscv-gnu-toolchain/bin:$PATH
export TARGET_PREFIX=riscv64-unknown-elf
```

The older `riscv-unknown-elf32-*` naming mentioned in `README_RISCV.md` is not
automatically probed, but works when selected through `TARGET_PREFIX`.

## Verifying a toolchain

Before starting an eLua build, check both the compiler and the requested C
library:

```sh
export CC=riscv64-unknown-elf-gcc
$CC -dumpmachine
$CC -march=rv32im_zicsr_zifencei -mabi=ilp32 -x c -c -o /tmp/rv32-check.o - <<<'int x;'
$CC -march=rv32im_zicsr_zifencei -mabi=ilp32 -print-file-name=libc.a
$CC -print-file-name=picolibc.specs
rm -f /tmp/rv32-check.o
```

For the last command, a usable Picolibc installation returns an existing path.
If it prints the literal `picolibc.specs`, that specs file is not installed for
this compiler.

## Required build-system improvement

The intended behaviour is to support both toolchain variants without editing
`conf.lua` or adding absolute library paths.  The build system should gain a
C-library selection such as:

```text
libc=auto | newlib | picolibc
```

`auto` should be the default and should probe the selected `comp.CC` using
`$CC -print-file-name=picolibc.specs`:

- If the result names an existing file, choose Picolibc and add
  `ELUA_PICOLIBC` plus `-specs=picolibc.specs`.
- Otherwise choose Newlib and retain eLua's startup/syscall setup and normal
  `-lc -lgcc -lm` linkage, without the Picolibc macro or specs flag.
- `libc=newlib` and `libc=picolibc` must override auto-detection.  The latter
  must fail early with a useful error if the selected compiler lacks
  `picolibc.specs`.

The probe must use the compiler selected for the current board, not the host
GCC, and must not assume a particular Debian package layout or hard-code a
sysroot path.

Until this is implemented, use a Picolibc-capable compiler for
`bonfire_arty_minimal` and `riscv32_spike`.  Newlib support is a documented
pending task, not a supported build path.
