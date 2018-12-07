-- Booatable eLua image build configuration

return {
  cpu = 'BONFIRE10',
  components = {
    sercon = { uart = 0, speed = 500000, buf_size=4096 },
    romfs = true,
    shell = {advanced=true},
    term = { lines = 25, cols = 80 },
    xmodem= true,
    linenoise = { shell_lines=20, lua_lines=20 },
    cints=true,
    luaints=true,
    bonfire_gdbserver= { uart = 1,  speed=500000 },
    editor=true
  },
  config = {
    vtmr= { num=4, freq=50 },
    i2c = { scl_bit=30, sda_bit=29 },
    bonfire= {
      num_uarts=2,
      uarts={"UART0_BASE","UART1_BASE"},
      uart_ints={"UART0_INTNUM","UART1_INTNUM"}
    
    }
  },
  modules = {
    generic = { 'pd', 'all_lua', 'term','uart','tmr','elua',"cpu","bit","pack","pio","i2c" },
    platform = { 'riscv','gdbserver' }
  },

  macros = {
     'ZPUINO_UART','OLD_ZPUINO_UART',
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

