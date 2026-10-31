-- Lua 5.5 eLua core-services module smoke test.
assert(type(elua) == "table")
assert(type(elua.version()) == "string")

local heap, in_use = elua.heapstats()
assert(type(heap) == "number")
assert(type(in_use) == "number")
