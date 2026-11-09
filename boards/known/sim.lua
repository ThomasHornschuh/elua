-- eLua simulator running on linux

return {
  cpu = 'linux',
  components = {
    sercon = { uart = 0, speed = 0 },
    wofs = true,
    romfs = true,
    shell = { advanced = true },
    term = { lines = 25, cols = 80 },
    linenoise = { shell_lines = 20, lua_lines = 50 },
    mmcfs = { spi = 0, cs_port = 0, cs_pin = 0 },
    cints = true,
    luaints = { queue_size = 32 },
  },
  config = {
    vtmr = { num = 4, freq = 10 },
  },
  modules = {
    generic = { 'pd', 'all_lua', 'term', 'elua', 'tmr', 'cpu' }
  },
  cpu_constants = {
    'INT_TMR_MATCH'
  }
}
