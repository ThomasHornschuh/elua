module( ..., package.seeall )




-- Add specific components to the 'components' table
function add_platform_components( t, board, cpu )


  t.bonfire_riscv = { macro = 'ENABLE_BONFIRE_RISCV' }
  t.bonfire_gdbserver = { macro= 'ENABLE_BONFIRE_GDBSERVER'}
end


-- Add specific configuration to the 'configs' table
function add_platform_configs( t, board, cpu )


  --t.lm3s_adc_timers = {
    --attrs = {
      --first_timer = at.int_attr( 'ADC_TIMER_FIRST_ID', 0 ),
      --num_timers = at.int_attr( 'ADC_NUM_TIMERS', 0 )
    --},
    --required = { first_timer = 0, num_timers = "NUM_TIMER" }
  --}
end

-- Return an array of all the available platform modules for the given cpu
function get_platform_modules( board, cpu )

  m = { riscv = {  lib = '"riscv"',  open = luaopen_riscv },
        gdbserver= { lib='"gdbserver"', map="gdbserver_map", open=false }
   }

  return m
end

