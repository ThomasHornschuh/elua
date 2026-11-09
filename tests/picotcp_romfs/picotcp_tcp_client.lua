-- PicoTCP-NG TAP regression client.  The Linux side is deliberately the
-- listener: this avoids accepting connections from a Lua callback context.
local peer = net.packip("192.168.254.1")
local client = assert(net.socket(net.SOCK_STREAM), "net.socket failed")

assert(net.connect(client, peer, 8080) == net.ERR_OK, "net.connect failed")
local connected_until = os.clock() + 1
while os.clock() < connected_until do net.tick() end

local sent, send_error = net.send(client, "elua-picotcp")
assert(sent == 12, "net.send failed: " .. tostring(send_error))
local flush_until = os.clock() + 1
while os.clock() < flush_until do net.tick() end

local response, receive_error = net.recv(client, 6, tmr.SYS_TIMER, 5000000)
assert(response == "tcp-ok", "net.recv failed: " .. tostring(receive_error) ..
       " response=" .. tostring(response))
net.close(client)
print("ELUA_PICOTCP_TCP_PASS")
