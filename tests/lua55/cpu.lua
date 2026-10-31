-- Lua 5.5 eLua CPU module smoke test.
assert(type(cpu) == "table")
assert(type(cpu.clock()) == "number")
