-- Configuration file for the RV32IM on bonfire Platform
-- Intended to run standalone


if comp.cpu:lower()=="bonfire_arty_10" then
  specific_files = sf( "start.S platform.c stubs.c uart.c systimer.c console.c platform_int.c mod_riscv.c xil_etherlite.c", comp.cpu:lower() )
else
   specific_files = sf( "start.S platform.c stubs.c uart.c systimer.c console.c platform_int.c mod_riscv.c", comp.cpu:lower() )
end
--local ldscript = "src/platform/bonfire/riscv_local.ld"
local ldscript = sf( "src/platform/%s/%s", platform, "riscv_local.ld" )

-- Override default optimize settings
delcf{ "-Os", "-fomit-frame-pointer" }
addcf{"-g", "-O2","-fomit-frame-pointer"}

-- Prepend with path
specific_files = utils.prepend_path( specific_files, sf( "src/platform/%s", platform ) )


-- Standard GCC flags

addcf{ '-march=rv32im' ,'-mabi=ilp32' }
addcf{ '-ffunction-sections', '-fdata-sections', '-fno-strict-aliasing', '-Wall'}
addlf{ '-nostartfiles', '-nostdlib', '-T', ldscript, '-Wl,--gc-sections', '-Wl,--allow-multiple-definition' }
addlib{ 'c','gcc','m' }



local gcc_version=utils.exec_capture(comp.CC.." -dumpversion")
print(string.format("Found %s version %s",comp.CC, gcc_version))
if gcc_version:sub(1,1)>="7" then -- For RISCV gcc version >= 7
  print("Configuring for gcc Version >= 7")
  addcf{'-mstrict-align','-mbranch-cost=6'}
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

