-- Booatable eLua image build configuration

return {
  cpu = 'BONFIRE_ARTY_10',
  components = {
    sercon = { uart = 0, speed = 500000,  buf_size=4096 },
    romfs = false,
    shell = true,
    advanced_shell=true,
    term = { lines = 25, cols = 80 },
    xmodem= true,
    linenoise = { shell_lines=20, lua_lines=20 },
    cints=true,
    luaints=true,
    tcpip = { ip = "192.168.26.200", netmask = "255.255.255.0", gw = "192.168.26.2", dns = "192.168.26.2" },
    dns=true,
    dhcp=true,
    mmcfs= { spi=0, cs_port=0, cs_pin=0 }
  },
  config = {
    vtmr= { num=4, freq=50 },
    bonfire= {
        num_uarts=2,
        uarts={"UART0_BASE","UART1_BASE"},
        uart_ints={"UART0_INTNUM","UART1_INTNUM"}
      
    }
  },
  modules = {
    generic = { 'pd', 'all_lua', 'term','uart','tmr','elua',"cpu","bit","net","pack","spi","pio" },
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

