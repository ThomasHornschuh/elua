-- Configuration file for the RV32IM on bonfire Platform
-- Intended to run standalone

--specific_files = sf( "boot.s utils.s hostif_%s.c platform.c host.c", comp.cpu:lower() )

if comp.cpu:lower()=="bonfire_arty_10" then
  specific_files = sf( "start.S platform.c stubs.c uart.c systimer.c console.c platform_int.c mod_riscv.c xil_etherlite.c", comp.cpu:lower() )
else
   specific_files = sf( "start.S platform.c stubs.c uart.c systimer.c console.c platform_int.c mod_riscv.c", comp.cpu:lower() )
end
local ldscript = "src/platform/bonfire/riscv_local.ld"

-- Override default optimize settings
--delcf{ "-Os", "-fomit-frame-pointer" }
--addcf{ "-O0", "-g" }
addcf{"-g", "-O3","-fomit-frame-pointer"}

-- Prepend with path
specific_files = utils.prepend_path( specific_files, sf( "src/platform/%s", platform ) )
--local ldscript = sf( "src/platform/%s/%s", platform, ldscript )

-- Standard GCC flags
addcf{ '-ffunction-sections', '-fdata-sections', '-fno-strict-aliasing', '-Wall'}
addlf{ '-nostartfiles', '-nostdlib', '-T', ldscript, '-Wl,--gc-sections', '-Wl,--allow-multiple-definition' }
--addlf{  '-Wl,--allow-multiple-definition','-Wl,--gc-sections' , '-T', ldscript }
addlib{ 'c','gcc','m' }

local target_flags = { '-march=rv32im' ,'-mabi=ilp32' } --, '-mstrict-align','-mbranch-cost=6'}
addcf{ target_flags}

--addcf{ target_flags, '-fno-builtin', '-fno-stack-protector' }
--addlf{ target_flags, '-Wl,-e,start', '-Wl,-static' }
--addaf{ '-felf' }

-- Also tell the builder that we don't need dependency checks for assembler files
builder:set_asm_dep_cmd( false )

-- Toolset data
tools.bonfire = {}

-- Programming function for i386 (not needed, empty function)
tools.bonfire.progfunc = function( target, deps )
  print "Run the simulator (./run_elua_sim.sh) and enjoy :) Linux only."
  return 0
end

-- Add the programming function explicitly for this target
tools.bonfire.pre_build = function()
  local t = builder:target( "#phony:prog", { exetarget }, tools.bonfire.progfunc )
  builder:add_target( t, "build eLua firmware image", { "prog" } )
end
