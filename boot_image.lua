require "pack"
bit32=require "bit32" -- from luarocks

local LOAD_BASE=0x010000

--[[
typedef struct {
  uint32_t magic;
  uint32_t nPages;  // File length in Number of 4096 Byte Units
  uint32_t loadAddress; // Address to load 
  uint32_t brkAddress;
  uint8_t  hash[32]; // SHA-256 Hash over code (without this header)
} t_flash_header_data;

typedef struct {
  t_flash_header_data header;
  uint8_t padding[256-sizeof(t_flash_header_data)];

} t_flash_header;

#define C_MAGIC 0x55aaddcc // new MAGIC...

]]


local f=assert(io.open(arg[1]..".bin","rb"))
local data = f:read("*a")
f:close()
local size=#data


local nPages=math.floor(size/4096)
if (size % 4096)>0 then
  nPages=nPages+1
end

-- brk_address= ((uint32_t)LOAD_BASE + recv_bytes + 4096) & 0x0fffffffc;
local brk_address= bit32.band(LOAD_BASE+size+4096, 0xfffffffc)

print(string.format("Filling header with nPages=%d, brkAddress=%x",nPages,brk_address))

local header=string.pack("<I<I<I<I",0x55aaddcc,nPages,LOAD_BASE,brk_address)
header=header..string.rep("\0",256-#header)
print(#header) -- debug only

local fw=assert(io.open(arg[1]..".img","wb"))
fw:write(header,data)
fw:close()
print(string.format("%d Bytes written",#header+#data))
