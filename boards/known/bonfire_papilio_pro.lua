-- Booatable eLua image build configuration

return {
  cpu = 'BONFIRE10',
  components = {
    sercon = { uart = 0, speed = 500000, buf_size=4096 },
    romfs = true,
    shell = true,
    term = { lines = 25, cols = 80 },
    xmodem= true,
    linenoise = { shell_lines=20, lua_lines=20 },
    cints=true,
    luaints=true
  },
  config = {
    vtmr= { num=4, freq=50 }
  },
  modules = {
    generic = { 'pd', 'all_lua', 'term','uart','tmr','elua',"cpu","bit","pack" },
    platform = { 'riscv','gdbserver' }
  },

  macros = {
     'ZPUINO_UART',
     {'UART_FIFO_THRESHOLD', 48 }
  },

  cpu_constants = {
    'INT_TMR_MATCH',
    'INT_UART_RX_FIFO',
    'GPIO_BASE',
    'MTIME_BASE',
    'SPIFLASH_BASE',
    'UART_BASE',
    'UART0_BASE',
    'UART1_BASE'
  }
}

