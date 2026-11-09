local net = require("net")
local tmr = require("tmr")

assert(net.lookup("192.168.254.1") == net.packip("192.168.254.1"))
assert(net.nameserver(net.packip("192.168.254.1"), "add") == -1)
assert(not pcall(net.lookup, "peer.invalid", function() end))
assert(not pcall(net.debug, 1, "/tmp/picotcp-debug.log"))

local server = assert(net.listen(8080), "net.listen failed")
print("ELUA_PICOTCP_TCP_READY")

local client, _, accept_error = net.accept(8080, tmr.SYS_TIMER, 15000000)
assert(client, "net.accept failed: " .. tostring(accept_error))
assert(not pcall(net.recv, client, "*l"))

local request = ""
while #request < 12 do
  local chunk, receive_error =
    net.recv(client, 12 - #request, tmr.SYS_TIMER, 5000000)
  assert(chunk and #chunk > 0,
         "net.recv failed: " .. tostring(receive_error))
  request = request .. chunk
end
assert(request == "elua-picotcp", "unexpected TCP request: " .. request)

local sent, send_error = net.send(client, "tcp-ok")
assert(sent == 6, "net.send failed: " .. tostring(send_error))
net.close(client)
net.close(server)
print("ELUA_PICOTCP_TCP_PASS")
