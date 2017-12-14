module( ..., package.seeall )

local at = require "attributes"

local function pt(t)

local level=0

  local function doprint(t)

    for k,v in pairs(t) do
      print(string.rep(" ",level) ..string.format("%s = %s",k,tostring(v)))
      if type(v)=="table"or type(v)=="romtable" then
        level=level+1
        doprint(v)
        level=level-1
      end
    end
  end

  doprint(t)
end


-- Add specific components to the 'components' table
function add_platform_components( t, board, cpu )


  t.bonfire_riscv = { macro = 'ENABLE_BONFIRE_RISCV' }
  t.bonfire_gdbserver = { macro= 'ENABLE_BONFIRE_GDBSERVER'}
end


-- Add specific configuration to the 'configs' table
function add_platform_configs( t, board, cpu )

    print(board,cpu)

  --if board:lower()=="bonfire_papilio_pro" then
    t.i2c = {
      attrs = {
        scl_bit = at.int_attr('I2C_SCL_BIT',0 ,31),
        sda_bit = at.int_attr('I2C_SDA_BIT', 0,31)
      }
    }
--  end
   pt(t)


end

-- Return an array of all the available platform modules for the given cpu
function get_platform_modules( board, cpu )

  m = { riscv = {  lib = '"riscv"',  open = luaopen_riscv },
        gdbserver= { lib='"gdbserver"', map="gdbserver_map", open=false }
   }

  return m
end

