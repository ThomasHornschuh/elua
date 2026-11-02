-- Configuration file for the RV32IM on bonfire Platform
-- Intended to run standalone

function _pt(t)
  for k,v in pairs(t) do print(k,v) end
end



specific_files = "start.S platform.c stubs.c  systimer.c console.c platform_int.c mod_riscv.c"


local board=comp.board:lower()
if board=="bonfire_papilio_pro" or board=="bonfire_ulx3s" then
   specific_files = specific_files .. " i2c.c oled-display/font.c oled-display/lcd.c oled-display/mod_oled.c"
--  specific_files = specific_files .. " socz80_uart.c"
end

if comp.cpu:lower()=="bonfire_arty_10" then
  -- The SPI core is needed for MMC/SD.  The Ethernet Lite driver is only
  -- needed when the board configuration selects an IP stack.
  specific_files = specific_files .. " xil_spi.c"
  if board ~= "bonfire_arty_minimal" then
    specific_files = specific_files .. " xil_etherlite.c"
  end
elseif comp.cpu:lower()=="bonfire_basic_soc_10" then
    specific_files = specific_files .. " bonfire_spi.c" 
end

specific_files = specific_files .. " gdb_interface.c riscv-gdb-stub.c mod_gdbserver.c"

local ldscript = sf( "src/platform/%s/%s", platform, "riscv_local.ld" )

-- Override default optimize settings
if not comp.debug then
 delcf{ "-Os" }
 addcf{"-g", "-O2", "-flto" }
end
-- Prepend with path
specific_files = utils.prepend_path( specific_files, sf( "src/platform/%s", platform ) )


-- Standard GCC flags

-- Recent GCC releases require CSR instructions to be requested explicitly.
addaf{ '-march=rv32im_zicsr_zifencei' ,'-mabi=ilp32' }
addcf{ '-march=rv32im_zicsr_zifencei' ,'-mabi=ilp32', '-DRISCV32' }
addcf{ '-ffunction-sections', '-fdata-sections', '-fno-strict-aliasing', '-Wall'}
addlf{ '-march=rv32im_zicsr_zifencei' ,'-mabi=ilp32' }
addlf{ '-nostartfiles', board == "bonfire_arty_minimal" and '-nodefaultlibs' or '-nostdlib',
       '-T', ldscript, '-Wl,--gc-sections', '-Wl,--allow-multiple-definition' }
-- Debian's RISC-V bare-metal compiler injects Newlib's libgloss for -lc.
-- This firmware uses Picolibc and eLua's own stubs, therefore link the
-- Picolibc archives directly for the minimal Arty profile.
local picolibc_sysroot = "/usr/lib/picolibc/riscv64-unknown-elf"
local picolibc_libdir = picolibc_sysroot .. "/lib/rv32im/ilp32"
if board == "bonfire_arty_minimal" then
  addlib{ picolibc_libdir .. '/libc.a', 'gcc', picolibc_libdir .. '/libm.a' }
else
  addlib{ 'c','gcc','m' }
end

if board == "bonfire_arty_minimal" then
  -- Debian's RISC-V bare-metal compiler is shipped without Newlib.  Use the
  -- installed Picolibc sysroot; eLua supplies its own start-up and syscall
  -- stubs for the Bonfire firmware.
  addcf{ '-DELUA_PICOLIBC', '--sysroot=' .. picolibc_sysroot,
         '-isystem', picolibc_sysroot .. '/include' }
  addlf{ '--sysroot=' .. picolibc_sysroot, '-L' .. picolibc_libdir,
         '-Wl,-u,stdin,-u,stdout,-u,stderr', '-nolibc' }
end

-- Keep the normal optimized build, but use a non-LTO link for Picolibc.
if board == "bonfire_arty_minimal" then
  delcf{ '-flto' }
end



local gcc_version=utils.exec_capture(comp.CC.." -dumpversion")
print(string.format("Found %s version %s",comp.CC, gcc_version))
if tonumber(gcc_version:match("%d+")) >=7 then -- For RISCV gcc version >= 7
  print("Configuring for gcc Version >= 7")
  addcf{'-mstrict-align' }
else
  print("Configuring for gcc Version < 7")
end


-- Also tell the builder that we don't need dependency checks for assembler files
--builder:set_asm_dep_cmd( false )

-- Toolset data
tools.bonfire = {}
--tools.bonfire.prog_flist = { output .. ".bin"}

-- Programming function for i386 (not needed, empty function)
--tools.bonfire.progfunc = function( target, deps )
  --print "bonfire.progfunc"
  --return 0
--end

-- Add the programming function explicitly for this target
--tools.bonfire.pre_build = function()
  --local t = builder:target( "#phony:prog", { exetarget }, tools.bonfire.progfunc )
  --builder:add_target( t, "build eLua firmware image", { "prog" } )
--end
