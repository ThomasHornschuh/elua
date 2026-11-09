---
name: build-elua-sim-target
description: Verify prerequisites, optionally synchronize, and build the native Lua 5.5 eLua sim target. Use when asked to build, prepare, diagnose, or report the local eLua Linux simulator target.
compatibility: Requires a Linux host, git, and permission to install missing host dependencies or pull origin updates when requested.
---

# Build eLua sim target

Build the native Linux `sim` board with the Lua 5.5 runtime. The build-script
interpreter is Lua 5.1; it is not the firmware Lua version.

## Safety and interaction rules

- Work from the repository root. Derive it with `git rev-parse --show-toplevel`
  and do not build from an unrelated directory.
- Preserve uncommitted changes. Never reset, clean, stash, commit, push, or
  change remotes.
- Do not install packages, install LuaRocks modules, run privileged commands,
  pull, merge, rebase, or otherwise change the checkout without explicit user
  approval in the current conversation.
- `git fetch` only updates remote-tracking metadata and is permitted for the
  origin-update check. All Git remote interaction must use `origin`, never
  `upstream`.
- If a required command is missing, report the missing prerequisites and exact
  proposed installation commands, then stop and ask for approval. Do not start
  a partial build.
- If `origin` is ahead, report the commits and ask whether to fast-forward
  pull them. Do not build until the user either approves the pull or explicitly
  asks to build the current local revision.
- If the local branch has diverged from `origin`, or the working tree is dirty
  and a pull is requested, stop and ask the user how to proceed. Do not attempt
  an automatic merge.

## 1. Check prerequisites

Check these commands and Lua 5.1 modules before any pull or build:

```sh
command -v gcc
command -v lua5.1
command -v git
lua5.1 -e 'require("lfs"); require("pack"); require("md5")'
```

`build-essential` provides the native compiler and linker on Debian/Ubuntu.
`lua5.1`, `liblua5.1-0-dev`, and `luarocks` provide the build-script runtime
and module installation path. The build itself does not need `mtools`, NASM,
32-bit multilib, or a RISC-V cross compiler.

When dependencies are missing on Debian/Ubuntu, propose (but do not run until
approved):

```sh
sudo apt update
sudo apt install build-essential lua5.1 liblua5.1-0-dev luarocks
sudo luarocks --lua-version=5.1 install luafilesystem
sudo luarocks --lua-version=5.1 install lpack
sudo luarocks --lua-version=5.1 install md5
```

On another distribution, report the missing commands/modules and ask the user
for the preferred package-manager commands instead of guessing. After approved
installation, rerun the complete prerequisite check.

## 2. Check origin updates

1. Record `git status --short --branch`.
2. Get the current branch with `git branch --show-current`; stop if detached.
3. Verify the branch has an `origin/<branch>` counterpart. If it does not,
   report that there is no origin branch to compare and ask whether to build
   the local revision.
4. Fetch only that branch:

   ```sh
   git fetch origin <branch>
   ```

5. Compare `HEAD` with `origin/<branch>`:
   - equal: continue;
   - origin ahead: show `git log --oneline HEAD..origin/<branch>` and ask for
     approval to run `git pull --ff-only origin <branch>`;
   - local ahead: report it and ask whether to build the local revision;
   - diverged: show both commit ranges and stop for an explicit user decision.

After an approved fast-forward pull, rerun prerequisite checks because project
requirements may have changed.

## 3. Build

Unless the user supplies different build/output directories, use an isolated
Lua 5.5 build directory and run from the repository root:

```sh
lua5.1 build_elua.lua board=sim lua_version=lua55 \
  build_mode=build_dir_linearized build_dir=.build-sim-lua55
```

Capture combined stdout/stderr in a temporary log while still streaming it to
the user. Do not hide compiler diagnostics. A successful builder exit alone is
not enough: verify that `elua_lua_sim.elf` exists and is executable.

If the build fails, report the exit status, the last relevant command/error
lines, the log path, and do not claim a target was produced.

## 4. Report warnings and target statistics

For a successful build:

1. Search the captured log for compiler/preprocessor/linker warnings (at least
   lines containing `warning:` or `#warning`).
2. State either **no warnings detected** or the warning count and each unique
   warning with its source context. Never describe a warning as harmless unless
   verified from the source or the user explicitly accepts it.
3. Report:
   - Git revision: `git rev-parse --short HEAD`;
   - target path and `file elua_lua_sim.elf` output;
   - byte size from `stat -c '%s' elua_lua_sim.elf` and human-readable size
     from `ls -lh elua_lua_sim.elf`;
   - section totals from `size elua_lua_sim.elf` when `size` is available;
   - SHA-256 from `sha256sum elua_lua_sim.elf` when available.

State clearly when an optional statistics command is unavailable rather than
installing it without approval.

## 5. Tell the user how to start it

Give commands appropriate to the requested shell:

```sh
# From the repository root
./elua_lua_sim.elf

# Optional terminal helper
./run_elua_sim.sh
```

Explain that `lua` enters the embedded Lua 5.5 REPL, Ctrl-D returns to the
eLua shell, and `exit` stops the simulator.

For SD-card tests, explain that the image must be named `sdcard.img` in the
simulator process's current working directory. Use a temporary directory to
avoid modifying the repository working directory:

```sh
mkdir -p /tmp/elua-lua55-tests
cp lua55-tests-sdcard.img /tmp/elua-lua55-tests/sdcard.img
cd /tmp/elua-lua55-tests
/path/to/elua/elua_lua_sim.elf
```

Then the eLua shell can run:

```text
lua /mmc/lua55/run.lua list
lua /mmc/lua55/run.lua all
```

Do not run the regression suite unless the user asks. If requested after a
successful build, use:

```sh
bash tests/run_lua55_sim_tests.sh
```
