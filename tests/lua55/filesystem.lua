-- Lua 5.5 eLua virtual-filesystem smoke test.
local path = "/rom/lua55_fileio.lua"
local stream = assert(io.open(path, "r"))
local source = assert(stream:read("*a"))
assert(stream:close())

assert(source:match("elua%-lua55%-fileio"))
assert(assert(loadfile(path))() == "elua-lua55-fileio")
