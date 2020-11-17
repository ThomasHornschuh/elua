-- Configuration file for the RV32IM on bonfire Platform
-- Intended to run standalone

function _pt(t)
  for k,v in pairs(t) do print(k,v) end
end



specific_files = "start.S platform.c stubs.c  systimer.c console.c platform_int.c mod_riscv.c"


local board=comp.board:lower()
if board=="bonfire_papilio_pro" or board=="bonfire_ulx3s" then
   specific_files = specific_files .. " i2c.c"
else
--  specific_files = specific_files .. " socz80_uart.c"
end

if comp.cpu:lower()=="bonfire_arty_10" then
  specific_files = specific_files .. " xil_etherlite.c xil_spi.c"
elseif comp.cpu:lower()=="bonfire_basic_soc_10" then
    specific_files = specific_files .. " bonfire_spi.c" 
end

-- Debug support
--if comp.board:lower()=="bonfire_papilio_pro" then
  specific_files = specific_files .. " gdb_interface.c riscv-gdb-stub.c mod_gdbserver.c"
--end

local ldscript = sf( "src/platform/%s/%s", platform, "riscv_local.ld" )

-- Override default optimize settings
if not comp.debug then
 delcf{ "-Os" }
 addcf{"-g", "-O2", "-flto" }
end
-- Prepend with path
specific_files = utils.prepend_path( specific_files, sf( "src/platform/%s", platform ) )


-- Standard GCC flags

addaf{ '-march=rv32im' ,'-mabi=ilp32' }
addcf{ '-march=rv32im' ,'-mabi=ilp32', '-DRISCV32' }
addcf{ '-ffunction-sections', '-fdata-sections', '-fno-strict-aliasing', '-Wall'}
addlf{ '-march=rv32im' ,'-mabi=ilp32' }
addlf{ '-nostartfiles', '-nostdlib', '-T', ldscript, '-Wl,--gc-sections', '-Wl,--allow-multiple-definition' }
addlib{ 'c','gcc','m' }



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

