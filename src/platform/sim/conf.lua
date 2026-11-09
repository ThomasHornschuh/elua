-- Configuration file for the linux (sim) backend

-- The simulator now uses the host glibc runtime.  In particular, do not link
-- its historical hand-written Newlib 'longjmp' implementation (utils.s): its
-- jmp_buf layout is incompatible with glibc and breaks modern Lua runtimes.
specific_files = sf( "hostif_%s.c platform.c host.c", comp.cpu:lower() )
if comp.board:lower() == "sim_picotcp" then
  specific_files = specific_files .. " " ..
    utils.prepend_path( "picotcp_tap.c", sf( "src/platform/%s", platform ) )
end
  
-- Override default optimize settings
delcf{ "-Os", "-fomit-frame-pointer" }
addcf{ "-O0", "-g" }

-- Prepend with path
specific_files = utils.prepend_path( specific_files, sf( "src/platform/%s", platform ) )

-- Standard GCC flags
addcf{ '-ffunction-sections', '-fdata-sections', '-fno-strict-aliasing', '-Wall' }
addlf{ '-Wl,--gc-sections', '-Wl,--allow-multiple-definition',
       [[-Wl,--defsym,stext=ADDR\(.text\)]],
       [[-Wl,--defsym,etext=ADDR\(.rodata\)+SIZEOF\(.rodata\)]] }
addlib{ 'm' }
addm({"_READ_WRITE_RETURN_TYPE=_ssize_t"})

-- The selected simulator source set contains only C files.  Build with the
-- native host compiler; do not force a host ISA, 32-bit ABI, or NASM.
addcf{ '-DELUA_SHELL_EOF_CTRL_D', '-DLUA_USE_POSIX', '-fno-pie', '-fno-builtin', '-fno-stack-protector' }
addlf{ '-no-pie' }

-- Toolset data
tools.sim = {}

-- Programming function for the native Linux simulator (not needed, empty function)
tools.sim.progfunc = function( target, deps )
  print "Run the simulator (./run_elua_sim.sh) and enjoy :) Linux only."
  return 0
end

-- Add the programming function explicitly for this target
tools.sim.pre_build = function()
  local t = builder:target( "#phony:prog", { exetarget }, tools.sim.progfunc )
  builder:add_target( t, "build eLua firmware image", { "prog" } )
end
