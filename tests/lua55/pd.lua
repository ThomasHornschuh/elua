-- Lua 5.5 eLua platform-data module smoke test.
assert(type(pd) == "table")
assert(pd.platform() == "SIM")
assert(pd.cpu() == "LINUX")
assert(pd.board() == "SIM")
