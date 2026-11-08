local tmr = require("tmr")
local sys = tmr.SYS_TIMER
local virt = tmr.VIRT0
local tick = 1000000 / tmr.getclock(virt)
local tolerance = 2000000

assert(sys and virt and tmr.VIRT1 and tmr.VIRT2 and tmr.VIRT3)
assert(tmr.VIRT4 == nil)
assert(tmr.getclock(sys) == 1000000)
assert(tmr.getmindelay(sys) == 1)
assert(tmr.getmaxdelay(sys) > 1000000)
assert(tmr.getmindelay(virt) == tick)

local start = tmr.start(sys)
tmr.delay(sys, 20000)
local finish = tmr.read(sys)
local elapsed = tmr.gettimediff(sys, start, finish)
assert(finish >= start)
assert(elapsed >= 20000 and elapsed < 20000 + tolerance)
assert(tmr.getdiffnow(sys, start) >= elapsed)

assert(tmr.start(virt) == 0)
tmr.delay(sys, 3 * tick)
local count = tmr.read(virt)
assert(count >= 2 and count <= 3 + tolerance / tick)

start = tmr.start(sys)
tmr.delay(virt, 2 * tick)
elapsed = tmr.getdiffnow(sys, start)
assert(elapsed >= 2 * tick and elapsed < 3 * tick + tolerance)
assert(tmr.read(virt) >= 2)

print("ELUA_LUA55_TIMERS_PASS")
