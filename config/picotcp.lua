
print("Configuring picotcp")

local PICODIR="picotcp/build_rv32" -- TODO: Make configurable

-- Link picotcp.a
addlf(sf( '-L%s/lib',PICODIR ))
addlib( 'picotcp' )

-- Picotcp include directory 
local includes='include include/arch'
includes= utils.prepend_path(includes,PICODIR)
addi( includes )

uip_files = ""