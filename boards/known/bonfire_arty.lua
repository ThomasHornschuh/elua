-- Booatable eLua image build configuration

return {
  cpu = 'BONFIRE_ARTY_10',
  components = {
    sercon = { uart = 0, speed = 500000 },
    romfs = true,
    shell = true,
    term = { lines = 25, cols = 80 },
    xmodem= true,
    linenoise = { shell_lines=20, lua_lines=20 },
    cints=true,
    luaints=true,
    tcpip = { ip = "192.168.26.200", netmask = "255.255.255.0", gw = "192.168.26.2", dns = "192.168.26.2" },
    dns=true,
    dhcp=true
  },
  config = {
    vtmr= { num=4, freq=50 }
  },
  modules = {
    generic = { 'pd', 'all_lua', 'term','uart','tmr','elua',"cpu","bit","net","pack" },
    platform = { 'riscv' }
  },
  cpu_constants = {
    'INT_TMR_MATCH',
    'GPIO_BASE',
    'MTIME_BASE',
    'SPIFLASH_BASE',
    'UART_BASE'
  }
}

