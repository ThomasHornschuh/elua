local cpu = require("cpu")

for _, operation in ipairs({
  function() return cpu.w32(0, 0) end,
  function() return cpu.r32(0) end,
  function() return cpu.w16(0, 0) end,
  function() return cpu.r16(0) end,
  function() return cpu.w8(0, 0) end,
  function() return cpu.r8(0) end,
}) do
  local ok, message = pcall(operation)
  assert(not ok)
  assert(string.find(message, "raw memory access is not available", 1, true))
end
