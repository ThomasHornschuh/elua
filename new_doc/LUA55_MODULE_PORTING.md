# Making eLua modules ready for Lua 5.5

This guide explains how to make an existing eLua C module work with the
Lua 5.5 runtime.  It is written both as an implementation guide for humans
and as an execution checklist for coding agents.

The goal is to keep one module implementation in `src/modules/` that builds
with both supported runtimes:

```sh
lua5.1 build_elua.lua board=sim lua_version=lua51
lua5.1 build_elua.lua board=sim lua_version=lua55
```

`lua5.1` in these commands is the host-side interpreter for the build script.
It does **not** select the Lua version embedded in eLua; `lua_version` does.

## Compatibility model

Lua 5.1 eLua modules often use eLua's Lua Tiny RAM (LTR) / ROM-table macros.
Lua 5.5 does not expose the VM internals required by those ROM tables.  The
Lua 5.5 build therefore sets `LUA_OPTIMIZE_MEMORY=0` and registers normal,
mutable Lua tables instead.

Do **not** create a parallel `modules55/` implementation for an otherwise
portable module.  Keep the module in `src/modules/` and use the existing
compatibility layer:

- `src/lua/lrotable.h` includes `inc/lua_compat.h` when `ELUA_LUA55` is set.
- `inc/lua_compat.h` provides the small Lua-5.1-to-5.5 API bridge currently
  used by eLua modules (`luaL_reg`, `luaL_register`, `lua_objlen`,
  `luaL_typerror`, `luaL_putchar`, and `lua_isrotable`).
- `src/lua/lrodefs.h` turns an LTR map into a normal `luaL_Reg` table whenever
  memory optimization is disabled.  `LREGISTER` then registers it through
  the compatibility layer.

For Lua 5.5, `lua_isrotable()` deliberately returns false: ROM tables do not
exist in that runtime.  Code that depends on a table being immutable or on
Lua-5.1 VM internals needs an explicit redesign, not just a macro alias.

## Worked example: `pd`

`pd` is a small module that exports platform metadata.  Its source is
`src/modules/pd.c`; the Lua-5.5 smoke test is `tests/lua55/pd.lua`.

The module uses an LTR-style declaration:

```c
#include "lrotable.h"

#define MIN_OPT_LEVEL 2
#include "lrodefs.h"

const LUA_REG_TYPE pd_map[] = {
  { LSTRKEY( "platform" ), LFUNCVAL( pd_platform ) },
  { LSTRKEY( "cpu" ),      LFUNCVAL( pd_cpu ) },
  { LSTRKEY( "board" ),    LFUNCVAL( pd_board ) },
  { LNILKEY, LNILVAL }
};

LUALIB_API int luaopen_pd( lua_State *L )
{
  LREGISTER( L, AUXLIB_PD, pd_map );
}
```

No `#ifdef ELUA_LUA55` is needed in `pd.c`:

1. With Lua 5.1 and sufficiently high `LUA_OPTIMIZE_MEMORY`, `lrodefs.h`
   keeps the ROM-table representation.
2. With Lua 5.5, `LUA_OPTIMIZE_MEMORY=0`, so the same entries become a normal
   `luaL_Reg` array and `LREGISTER` calls the compatibility implementation of
   `luaL_register`.
3. The Lua-visible API remains `pd.platform()`, `pd.cpu()`, and `pd.board()`.

The test is intentionally small and board-specific:

```lua
assert(type(pd) == "table")
assert(pd.platform() == "SIM")
assert(pd.cpu() == "LINUX")
assert(pd.board() == "SIM")
```

Build and run it together with the curated Lua 5.5 regression suite:

```sh
lua5.1 build_elua.lua board=sim lua_version=lua55 \
  build_mode=build_dir_linearized build_dir=.build-sim-lua55
bash tests/run_lua55_sim_tests.sh
```

## Porting procedure

### 1. Inventory the module's Lua-facing code

Read the complete module before editing.  Identify:

- its `luaopen_<module>` function and exported table(s);
- Lua 5.1 compatibility APIs (`luaL_register`, `lua_objlen`,
  `luaL_typerror`, etc.);
- LTR/ROM-table macros and direct Lua VM internal headers;
- userdata, metatable, coroutine, allocator, filesystem, and platform calls;
- compile-time guards such as `BUILD_TERM` or `NUM_SPI > 0`.

Classify every use as either covered by `inc/lua_compat.h`, safely replaced by
the public Lua 5.5 API, or requiring a runtime-specific implementation.

### 2. Reuse the existing source whenever possible

For a conventional map-based module, retain this shape:

```c
#include "lualib.h"
#include "lauxlib.h"
#include "lrotable.h"  /* also selects eLua's Lua-5.5 compatibility layer */

/* C functions returning the number of Lua results. */

#define MIN_OPT_LEVEL 2
#include "lrodefs.h"

const LUA_REG_TYPE module_map[] = {
  { LSTRKEY("function"), LFUNCVAL(module_function) },
  { LNILKEY, LNILVAL }
};

LUALIB_API int luaopen_module(lua_State *L)
{
  LREGISTER(L, AUXLIB_MODULE, module_map);
}
```

When a module does not use `lrotable.h`, include `lua_compat.h` after the Lua
public headers, guarded by `ELUA_LUA55` if needed.  Prefer public Lua APIs;
do not include Lua-5.1 internals such as `lstate.h`, `lobject.h`, or
`ltable.h` in code that must support Lua 5.5.

### 3. Handle non-portable features explicitly

Use a small, documented conditional only where behavior genuinely differs:

```c
#if defined(ELUA_LUA55)
  /* Lua 5.5 implementation using public API only. */
#else
  /* Existing Lua-5.1-only optimization, if still required. */
#endif
```

Typical cases are old garbage-collector hooks, direct `TValue` access,
rotable-specific inspection, and assumptions about 5.1's environment model.
Do not hide an incompatible implementation behind a broad macro alias.

### 4. Register and select the module

For a new generic eLua module, keep the build metadata in sync:

1. Add the module name and any feature guards to `elua_generic_modules` in
   `config/modules.lua`.
2. Add `AUXLIB_<MODULE>` and the `luaopen_<module>` declaration to
   `src/modules/auxmods.h`.
3. Put the implementation in `src/modules/<module>.c` and use the matching
   `AUXLIB_<MODULE>` name in its open function.
4. Add the module to the selected board's `modules.generic` list in
   `boards/known/<board>.lua` (or enable it through an existing `all_lua` /
   `all_elua` selection).

The generated board header determines which module open functions are placed
in the firmware image.  A source file compiling successfully is not proof
that its Lua module was registered.

### 5. Test both the C build and the Lua contract

At minimum:

```sh
# Existing runtime must remain buildable.
lua5.1 build_elua.lua board=sim lua_version=lua51 \
  build_mode=build_dir_linearized build_dir=.build-sim-lua51

# New runtime and its regression runner.
lua5.1 build_elua.lua board=sim lua_version=lua55 \
  build_mode=build_dir_linearized build_dir=.build-sim-lua55
bash tests/run_lua55_sim_tests.sh
```

Add a focused Lua smoke test under `tests/lua55/`.  It should verify module
registration, return types, representative valid arguments, error behavior,
and one target-specific observable result.  Extend
`tests/run_lua55_sim_tests.sh` when the test should run automatically.

Also test the target board that uses the hardware API.  The `sim` board is a
fast compatibility gate; it cannot validate real peripheral behavior.

## Coding-agent checklist

Before reporting completion, a coding agent should be able to answer all of
the following with paths and command output:

- Which old Lua API calls were found, and which compatibility mapping or
  public Lua-5.5 replacement handles each one?
- Does the module avoid Lua-5.1 VM internals in the Lua-5.5 path?
- Is the module named in `config/modules.lua`, `auxmods.h`, and the relevant
  board configuration when it is new?
- Does `lua_version=lua51` still build?
- Does `lua_version=lua55` build and pass its module smoke test?
- If hardware is involved, what was tested on real hardware and what remains
  simulator-only?

Keep changes narrow: do not duplicate an entire module merely to support Lua
5.5, and do not change a module's Lua-visible API unless that incompatibility
is explicit, documented, and tested.
