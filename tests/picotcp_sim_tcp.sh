#!/usr/bin/env bash
set -Eeuo pipefail

root=$(cd -- "$(dirname -- "${BASH_SOURCE[0]}")/.." && pwd)
sim=${ELUA_SIM:-"$root/elua_lua_sim_picotcp.elf"}
tap_name=${PICO_TAP_NAME:-"epico$$"}
tap_created=0
sim_pid=
tmpdir=$(mktemp -d)

fail() {
  printf 'FAIL: %s\n' "$*" >&2
  if [[ -f $tmpdir/sim.log ]]; then
    cat "$tmpdir/sim.log" >&2
  fi
  exit 1
}

cleanup() {
  local result=0
  if [[ -n $sim_pid ]]; then
    kill "$sim_pid" 2>/dev/null || true
    wait "$sim_pid" 2>/dev/null || true
    sim_pid=
  fi
  if (( tap_created )); then
    if ip link delete dev "$tap_name"; then
      tap_created=0
    else
      printf 'FAIL: could not remove temporary TAP interface %s\n' "$tap_name" >&2
      result=1
    fi
  fi
  rm -rf "$tmpdir"
  return "$result"
}
trap cleanup EXIT

if [[ $(uname -s) != Linux ]]; then
  fail "Linux is required"
fi
if (( EUID != 0 )); then
  fail "run as root (CAP_NET_ADMIN and permission to attach to TAP are required)"
fi
if [[ ! -c /dev/net/tun ]]; then
  fail "/dev/net/tun is unavailable"
fi
for command in ip python3 timeout; do
  command -v "$command" >/dev/null 2>&1 || fail "required command '$command' is unavailable"
done
[[ -x $sim ]] || fail "simulator executable not found: $sim"
[[ $tap_name =~ ^[a-zA-Z0-9_.-]{1,15}$ ]] || fail "PICO_TAP_NAME must be a 1-15 character Linux interface name"
if ip link show dev "$tap_name" >/dev/null 2>&1; then
  fail "interface $tap_name already exists"
fi

ip tuntap add dev "$tap_name" mode tap ||
  fail "could not create TAP interface; check CAP_NET_ADMIN"
tap_created=1
ip address add 192.168.254.1/30 dev "$tap_name" ||
  fail "could not configure private address on $tap_name"
ip link set dev "$tap_name" up ||
  fail "could not bring $tap_name up"

printf 'lua /rom/picotcp_tcp_server.lua\nexit\n' > "$tmpdir/input"
timeout --signal=TERM --kill-after=2s 25s env \
  ELUA_PICO_TAP="$tap_name" \
  ELUA_PICO_TAP_IP=192.168.254.2 \
  ELUA_PICO_TAP_NETMASK=255.255.255.252 \
  "$sim" < "$tmpdir/input" > "$tmpdir/sim.log" 2>&1 &
sim_pid=$!

for (( attempt = 0; attempt < 100; ++attempt )); do
  kill -0 "$sim_pid" 2>/dev/null || fail "simulator exited before starting the TCP server"
  if grep -Fq 'ELUA_PICOTCP_TCP_READY' "$tmpdir/sim.log"; then
    break
  fi
  sleep 0.1
done
grep -Fq 'ELUA_PICOTCP_TCP_READY' "$tmpdir/sim.log" ||
  fail "simulator did not start its TCP server within 10 seconds"

python3 - 192.168.254.2 > "$tmpdir/peer.log" 2>&1 <<'PY' ||
  fail "Linux peer TCP exchange failed"
import socket
import sys
import time

peer = sys.argv[1]
deadline = time.monotonic() + 10
last_error = None
while time.monotonic() < deadline:
    try:
        connection = socket.create_connection((peer, 8080), timeout=1)
        break
    except OSError as error:
        last_error = error
        time.sleep(0.1)
else:
    raise SystemExit(f"could not connect to eLua TCP server: {last_error}")

with connection:
    connection.settimeout(5)
    connection.sendall(b"elua-picotcp")
    response = bytearray()
    while len(response) < 6:
        chunk = connection.recv(6 - len(response))
        if not chunk:
            raise SystemExit("peer closed before returning the complete response")
        response.extend(chunk)
    if response != b"tcp-ok":
        raise SystemExit(f"unexpected TCP response: {bytes(response)!r}")
print("LINUX_PEER_TCP_PASS")
PY
grep -Fq 'LINUX_PEER_TCP_PASS' "$tmpdir/peer.log" ||
  fail "Linux peer did not report a successful TCP exchange"
cat "$tmpdir/peer.log"

wait "$sim_pid" || fail "simulator exited with an error"
sim_pid=
grep -Fq 'ELUA_PICOTCP_TCP_PASS' "$tmpdir/sim.log" ||
  fail "eLua did not report a successful TCP exchange"
cleanup
trap - EXIT
if ip link show dev "$tap_name" >/dev/null 2>&1; then
  fail "temporary TAP interface $tap_name still exists after cleanup"
fi
printf 'PASS: TCP request/response exchanged over TAP %s\n' "$tap_name"
