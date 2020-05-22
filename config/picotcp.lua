
print("Configuring picotcp")

local PICODIR
if comp.debug then
    PICODIR="picotcp/build_rv32_debug" -- TODO: Make configurable
else
    PICODIR="picotcp/build_rv32" -- TODO: Make configurable
end
print("Using: ",PICODIR)
-- Link picotcp.a
addlf(sf( '-L%s/lib',PICODIR ))
addlib( 'picotcp' )

-- Picotcp include directory 
local includes='include include/arch'
includes= utils.prepend_path(includes,PICODIR)
addi( includes )

uip_files = ""