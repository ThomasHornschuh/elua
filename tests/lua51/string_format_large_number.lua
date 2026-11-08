-- Regression test for Lua 5.1 string.format output exceeding 128 bytes.
-- Run with the Lua 5.1 runtime, for example: src/lua/lua tests/lua51/string_format_large_number.lua

for _, exponent in ipairs({ 126, 200, 308 }) do
  local formatted = string.format("%8.2f", -(10 ^ exponent))
  assert(#formatted > 128)
  assert(formatted:sub(1, 1) == "-")
end

print("ELUA_LUA51_STRING_FORMAT_LARGE_NUMBER_PASS")
