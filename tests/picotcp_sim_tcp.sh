#!/usr/bin/env bash
set -Eeuo pipefail

root=$(cd -- "$(dirname -- "${BASH_SOURCE[0]}")/.." && pwd)
sim=${ELUA_SIM:-"$root/elua_lua_sim_picotcp.elf"}
tap_name=${PICO_TAP_NAME:-"epico$$"}
tap_created=0
sim_pid=
peer_pid=
tmpdir=$(mktemp -d)

fail() {
  printf 'FAIL: %s\n' "$*" >&2
  [[ -f $tmpdir/peer.log ]] && cat "$tmpdir/peer.log" >&2
  [[ -f $tmpdir/sim.log ]] && cat "$tmpdir/sim.log" >&2
  exit 1
}
cleanup() {
  [[ -n $sim_pid ]] && { kill "$sim_pid" 2>/dev/null || true; wait "$sim_pid" 2>/dev/null || true; }
  [[ -n $peer_pid ]] && { kill "$peer_pid" 2>/dev/null || true; wait "$peer_pid" 2>/dev/null || true; }
  (( tap_created )) && ip link delete dev "$tap_name" || true
  rm -rf "$tmpdir"
}
trap cleanup EXIT

[[ $(uname -s) == Linux ]] || fail "Linux is required"
(( EUID == 0 )) || fail "run as root (CAP_NET_ADMIN and TAP access are required)"
[[ -c /dev/net/tun ]] || fail "/dev/net/tun is unavailable"
for command in ip python3 timeout; do command -v "$command" >/dev/null || fail "required command '$command' is unavailable"; done
[[ -x $sim ]] || fail "simulator executable not found: $sim"
[[ $tap_name =~ ^[a-zA-Z0-9_.-]{1,15}$ ]] || fail "invalid PICO_TAP_NAME"

ip tuntap add dev "$tap_name" mode tap
tap_created=1
ip address add 192.168.254.1/30 dev "$tap_name"
ip link set dev "$tap_name" up

# The host is the TCP listener.  This exercises eLua's outgoing TCP path
# without Lua-side accept/callback lifetime concerns.
ELUA_TEST_TMPDIR="$tmpdir" python3 - >"$tmpdir/peer.log" 2>&1 <<'PY' &
import os
import socket
server = socket.socket(socket.AF_INET, socket.SOCK_STREAM)
server.setsockopt(socket.SOL_SOCKET, socket.SO_REUSEADDR, 1)
server.bind(("192.168.254.1", 8080))
server.listen(1)
open(os.path.join(os.environ["ELUA_TEST_TMPDIR"], "peer-ready"), "w").close()
server.settimeout(15)
connection, _ = server.accept()
with connection:
    connection.settimeout(5)
    request = connection.recv(12)
    if request != b"elua-picotcp":
        raise SystemExit(f"unexpected request: {request!r}")
    connection.sendall(b"tcp-ok")
print("LINUX_PEER_TCP_PASS")
PY
peer_pid=$!
for _ in $(seq 1 100); do
  [[ -e $tmpdir/peer-ready ]] && break
  kill -0 "$peer_pid" 2>/dev/null || fail "Linux TCP server exited before listening"
  sleep 0.1
done
[[ -e $tmpdir/peer-ready ]] || fail "Linux TCP server did not begin listening"

printf 'lua /rom/picotcp_tcp_client.lua\nexit\n' > "$tmpdir/input"
timeout --signal=TERM --kill-after=2s 25s env \
  ELUA_PICO_TAP="$tap_name" ELUA_PICO_TAP_IP=192.168.254.2 \
  ELUA_PICO_TAP_NETMASK=255.255.255.252 \
  "$sim" < "$tmpdir/input" > "$tmpdir/sim.log" 2>&1 &
sim_pid=$!

wait "$peer_pid" || fail "Linux TCP server exchange failed"
peer_pid=
wait "$sim_pid" || fail "simulator exited with an error"
sim_pid=
grep -Fq LINUX_PEER_TCP_PASS "$tmpdir/peer.log" || fail "Linux peer did not report success"
grep -Fq ELUA_PICOTCP_TCP_PASS "$tmpdir/sim.log" || fail "eLua did not report success"
printf 'PASS: TCP request/response exchanged over TAP %s\n' "$tap_name"
