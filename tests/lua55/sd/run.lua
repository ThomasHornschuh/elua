-- Run the curated Lua 5.5 regression selection from the simulated SD card.
-- Usage: lua /mmc/lua55/run.lua [all|list|<test-name>]

local root = "/mmc/lua55"
local tests = {
  { "literals", "upstream/literals.lua" },
  { "constructs", "upstream/construct.lua" },
  { "calls", "upstream/calls.lua" },
  { "strings", "upstream/strings.lua" },
  { "utf8", "upstream/utf8.lua" },
  { "math", "upstream/math.lua" },
  { "vararg", "upstream/vararg.lua" },
  { "closure", "upstream/closure.lua" },
  { "events", "upstream/events.lua" },
  { "coroutine", "upstream/coroutin.lua" },
  { "pd", "elua/pd.lua" },
  { "elua", "elua/elua.lua" },
  { "term", "elua/term.lua" },
  { "filesystem", "elua/files.lua" },
}

local selection = arg[1] or "all"
if selection == "list" then
  for _, test in ipairs(tests) do print(test[1]) end
  return
end

local ran = 0
for _, test in ipairs(tests) do
  if selection == "all" or selection == test[1] then
    print("\n=== " .. test[1] .. " ===")
    dofile(root .. "/" .. test[2])
    print("PASS: " .. test[1])
    ran = ran + 1
  end
end

assert(ran > 0, "unknown test '" .. selection .. "' (use 'list')")
print("\nAll selected tests passed.")
