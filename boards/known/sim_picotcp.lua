-- Linux simulator with opt-in PicoTCP-NG TAP networking.

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
    tcpip = {
      stack = "picotcp",
      ip = "192.168.254.2",
      netmask = "255.255.255.0",
      gw = "0.0.0.0",
      dns = "0.0.0.0",
    },
  },
  config = {
    vtmr = { num = 4, freq = 10 },
  },
  modules = {
    generic = { 'pd', 'all_lua', 'term', 'elua', 'tmr', 'cpu', 'net' }
  }
}
