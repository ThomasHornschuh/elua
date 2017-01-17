-- Configuration file for the RV32IM on,  Spike Simulator
-- Intended to run with the Proxy Kernel

--specific_files = sf( "boot.s utils.s hostif_%s.c platform.c host.c", comp.cpu:lower() )
specific_files = sf( "platform.c stubs.c hostif_linux.c syscalls.c   ", comp.cpu:lower() )
local ldscript = "src/platform/riscv32spike/riscv_local.ld"
  
-- Override default optimize settings
--delcf{ "-Os", "-fomit-frame-pointer" }
--addcf{ "-O0", "-g" }

-- Prepend with path
specific_files = utils.prepend_path( specific_files, sf( "src/platform/%s", platform ) )
--local ldscript = sf( "src/platform/%s/%s", platform, ldscript ) 

-- Standard GCC flags
addcf{ '-ffunction-sections', '-fdata-sections', '-fno-strict-aliasing', '-Wall'}
--addlf{ '-nostartfiles', '-nostdlib', '-T', ldscript, '-Wl,--gc-sections', '-Wl,--allow-multiple-definition' }
addlf{  '-Wl,--allow-multiple-definition','-Wl,--gc-sections' , '-T', ldscript } 
addlib{ 'c','gcc','m' }

local target_flags = { '-march=rv32im' } -- '-mabi=ilp32'
addcf{ target_flags}

--addcf{ target_flags, '-fno-builtin', '-fno-stack-protector' }
--addlf{ target_flags, '-Wl,-e,start', '-Wl,-static' }
--addaf{ '-felf' }

-- Also tell the builder that we don't need dependency checks for assembler files
builder:set_asm_dep_cmd( false )

-- Toolset data
tools.riscv32spike = {}

-- Programming function for i386 (not needed, empty function)
tools.riscv32spike.progfunc = function( target, deps )
  print "Run the simulator (./run_elua_sim.sh) and enjoy :) Linux only."
  return 0
end

-- Add the programming function explicitly for this target
tools.riscv32spike.pre_build = function()
  local t = builder:target( "#phony:prog", { exetarget }, tools.riscv32spike.progfunc )
  builder:add_target( t, "build eLua firmware image", { "prog" } )
end

