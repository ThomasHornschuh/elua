-- Booatable eLua image build configuration

return {
  cpu = 'RV32IMSPIKE',
  components = {
    sercon = { uart = 0, speed = 0 },
    romfs = true,
    shell = true,
    term = { lines = 25, cols = 80 },
    xmodem= false
  },
  modules = {
    generic = { 'pd', 'all_lua', 'term','bit','elua','pack','cpu' }
  }
}

