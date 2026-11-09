
local source_dir = "picotcp"
local build_dir = "build-elua-sim"
local revision = "d2f46200b0768bb1d08aff6ed575abeacf8cb557"

local function run(command, description)
  local result = os.execute(command)
  if result ~= 0 then
    print(string.format("[CONFIG] ERROR: %s failed; see the command output above.", description))
    os.exit(1)
  end
end

if not utils.is_dir(source_dir) then
  print("[CONFIG] Fetching pinned PicoTCP-NG source (GPL-2.0-only OR GPL-3.0-only).")
  run("git clone https://github.com/virtualsquare/picotcp.git " .. source_dir, "PicoTCP-NG clone")
elseif not utils.is_dir(source_dir .. "/.git") then
  print("[CONFIG] ERROR: " .. source_dir .. " exists but is not a PicoTCP-NG checkout.")
  os.exit(1)
end

run("git -C " .. source_dir .. " checkout --detach " .. revision, "PicoTCP-NG revision checkout")

local checked_out = utils.exec_capture("git -C " .. source_dir .. " rev-parse HEAD")
if checked_out ~= revision then
  print("[CONFIG] ERROR: expected PicoTCP-NG revision " .. revision .. ", got " .. checked_out)
  os.exit(1)
end

print("WARNING: PicoTCP-NG is GPL-2.0-only OR GPL-3.0-only; consider the applicable GPL terms before distributing binaries that include it.")
print("[CONFIG] Building PicoTCP-NG for the native Linux simulator.")
run(string.format(
  "make -C %s PREFIX=%s CC=%s DEBUG=%d TCP=1 UDP=1 ETH=1 IPV4=1 IPV4FRAG=1 ICMP4=1 " ..
  "MCAST=0 NAT=0 DEVLOOP=0 DHCP_CLIENT=0 DHCP_SERVER=0 DNS_CLIENT=0 MDNS=0 TFTP=0 " ..
  "AODV=0 OLSR=0 SNTP_CLIENT=0 IPV6=0 TUN=0 TAP=0 VDE=0 PCAP=0 PPP=0 " ..
  "6LOWPAN=0 PACKET_SOCKET=0 RAW=0 lib",
  source_dir, build_dir, comp.CC, comp.debug and 1 or 0
), "PicoTCP-NG build")

local prefix = source_dir .. "/" .. build_dir
addlf(sf("-L%s/lib", prefix))
addlib("picotcp")
addi(prefix .. "/include")

uip_files = ""