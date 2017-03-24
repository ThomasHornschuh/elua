
eLua on RISC-V
==============

This is some inital effort to port eLua to the RISCV architecture. There are currently two variants:

 * RISCV32_spike 
   As a rough "proof-of-concept" I ported eLua to run with the proxy kernel on the RISC-V Spike simulator. The port is derived from the "sim" platform (elua/src/platform/sim) which runs eLua under LINUX as process, simulating the eLua console with LINUX system calls. Because the PK is also using a LINUX like syscall interface, it was very straigtforward to port this.
   This variant is built with:  lua5.1 build_elua.lua board=RISCV32_spike
   
 * bonfire_papilio_pro
   This variant runs on the Bonfire-SOC on a Papilio Pro FPGA Board (http://papilio.cc/index.php?n=Papilio.PapilioPro). I will plan to do some more documentation soon. 
   This Variant is built with:   lua5.1 build_elua.lua board=bonfire_papilio_pro
   
   
### Building eLua:   
More information how to build eLua can be found here here: http://www.eluaproject.net/doc/master/en_building.html
Unfortunately there is not much information about the prerequisites. 


The build system is not using make, it uses Lua. So Lua must be installed on the system, but it works only with Lua5.1 ! I don't unfortunately don't remember all the steps to get the Build system running, basically you need to install "Luarocks" and then some Luarocks modules. The build system will fail and complain what it is missing. It was not to hard, I managed to get it running with a few google searches, etc in 1-2 hours :-)

#### Other dependencies:
Of course you need a RISC-V toolchain installed and in your path. Because I aim for RV32IM only, I have build the RISC-V toolchain with the options --with-arch=rv32i and --with-abi=ilp32. This leads to gcc binaries prefixed with "riscv-unknown-elf32*". If your toolchain is named differently you can adapt the build_data-lua file in the eLua root directory, I think the file is mainly self-explanatory.


#### Running the RISCV_32_spike Image
To run RISCV32_spike you need to have spike and the the proxy Kernel for RV32I. The command to run eLua is:
	spike --isa=RV32IM -m256 /opt/riscv/riscv32-unknown-elf/bin/pk ./elua_lua_riscv32_spike.elf
(your may need to adapt the path to the pk...)

#### Trying out eLua
The image contains a Game of life program in the "romfs". Just type lua /rom/life.lua in the eLua shell and enjoy :-)
Don't be confused with that eLua states to be version 0.9 while the build system is for version 0.10. I think this is normal...


#### Running on Bonfire SoC
.... comming soon :-)

#### Bugs
The Spike eLua states to be v0.9_bonfire_RV32IM-2-g146707e. This is because the message is created by a Git Verson tag, and not
by the platform/board combination.






