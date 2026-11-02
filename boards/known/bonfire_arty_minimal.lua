-- Minimal eLua configuration for the Bonfire Arty board.
--
-- This deliberately excludes TCP/IP/PicoTCP and the Lua net module while
-- retaining the serial console, timer and UART APIs, plus FATFS on MMC/SD.

return {
  cpu = 'BONFIRE_ARTY_10',
  components = {
    sercon = { uart = 0, speed = 500000, buf_size = 4096 },
    shell = { advanced = true },
    term = { lines = 25, cols = 80 },
    linenoise = { shell_lines = 20, lua_lines = 20 },
    -- Required by the buffered serial console.
    cints = true,
    -- Keep the second UART available for the Bonfire GDB remote stub.
    bonfire_gdbserver = { uart = 1, speed = 500000 },
    mmcfs = { spi = 0, codepage = 850,
              use_utf8_api = false, tiny = false, use_locking = true,
              files = 32, lfn_length = 255 },
  },
  config = {
    vtmr = { num = 4, freq = 50 },
    bonfire = {
      num_uarts = 2,
      uarts = { "UART0_BASE", "UART1_BASE" },
      uart_ints = { "UART0_INTNUM", "UART1_INTNUM" }
    }
  },
  modules = {
    -- `pd`, all standard Lua libraries, `term`, and `elua` match `sim`.
    -- UART and timer access are added for the Arty hardware.
    generic = { 'pd', 'all_lua', 'term', 'elua', 'uart', 'tmr' },
    platform = { 'riscv', 'gdbserver' }
  },
  macros = {
    'ZPUINO_UART',
    { 'UART_FIFO_THRESHOLD', 48 }
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
