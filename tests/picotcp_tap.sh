#!/usr/bin/env bash
set -Eeuo pipefail

repo_root=$(cd -- "$(dirname -- "${BASH_SOURCE[0]}")/.." && pwd)
tap_name=${PICO_TAP_NAME:-pico$$}
host_ip=${PICO_TAP_HOST:-192.168.254.1}
stack_ip=${PICO_TAP_STACK:-192.168.254.2}
netmask=${PICO_TAP_NETMASK:-255.255.255.252}
picoapp=${PICOAPP:-"$repo_root/picotcp/build-host/test/picoapp.elf"}
tap_created=0
stack_pid=

fail() {
  printf 'FAIL: %s\n' "$*" >&2
  exit 1
}

valid_ipv4() {
  local address=$1 octet
  local -a octets

  [[ $address =~ ^[0-9]{1,3}(\.[0-9]{1,3}){3}$ ]] || return 1
  IFS=. read -r -a octets <<< "$address"
  for octet in "${octets[@]}"; do
    (( 10#$octet <= 255 )) || return 1
  done
}

mask_prefix() {
  local octet partial=0 prefix=0
  local -a octets

  IFS=. read -r -a octets <<< "$netmask"
  for octet in "${octets[@]}"; do
    case $octet in
      255) (( partial == 0 )) || return 1; prefix=$((prefix + 8)) ;;
      254) (( partial == 0 )) || return 1; prefix=$((prefix + 7)); partial=1 ;;
      252) (( partial == 0 )) || return 1; prefix=$((prefix + 6)); partial=1 ;;
      248) (( partial == 0 )) || return 1; prefix=$((prefix + 5)); partial=1 ;;
      240) (( partial == 0 )) || return 1; prefix=$((prefix + 4)); partial=1 ;;
      224) (( partial == 0 )) || return 1; prefix=$((prefix + 3)); partial=1 ;;
      192) (( partial == 0 )) || return 1; prefix=$((prefix + 2)); partial=1 ;;
      128) (( partial == 0 )) || return 1; prefix=$((prefix + 1)); partial=1 ;;
      0) partial=1 ;;
      *) return 1 ;;
    esac
  done
  (( prefix >= 1 && prefix <= 30 )) || return 1
  printf '%s\n' "$prefix"
}

ipv4_to_int() {
  local a b c d
  IFS=. read -r a b c d <<< "$1"
  printf '%u\n' "$(( (10#$a << 24) | (10#$b << 16) | (10#$c << 8) | 10#$d ))"
}

stop_stack() {
  if [[ -n $stack_pid ]]; then
    kill "$stack_pid" 2>/dev/null || true
    wait "$stack_pid" 2>/dev/null || true
    stack_pid=
  fi
}

cleanup() {
  local status=0
  stop_stack
  if (( tap_created )); then
    if ip link delete dev "$tap_name"; then
      tap_created=0
    else
      status=1
    fi
  fi
  return "$status"
}

# shellcheck disable=SC2317
on_exit() {
  local status=$?
  trap - EXIT
  if ! cleanup; then
    printf 'FAIL: unable to remove TAP interface %s\n' "$tap_name" >&2
    status=1
  fi
  exit "$status"
}
trap on_exit EXIT
trap 'exit 130' INT
trap 'exit 143' TERM

if [[ $(uname -s) != Linux ]]; then
  fail "Linux is required for TAP validation"
fi
if (( EUID != 0 )); then
  fail "run with sudo/root (CAP_NET_ADMIN and CAP_NET_RAW are required)"
fi
if [[ ! -c /dev/net/tun ]]; then
  fail "/dev/net/tun is unavailable; enable the Linux TUN/TAP device"
fi
for command in ip ping; do
  command -v "$command" >/dev/null 2>&1 || fail "required command '$command' is unavailable"
done
[[ -x $picoapp ]] || fail "missing executable $picoapp; build it as documented in doc/picotcp-ng.md"
[[ $tap_name =~ ^[a-zA-Z0-9_.-]{1,15}$ ]] || fail "PICO_TAP_NAME must be 1-15 valid Linux interface characters"
valid_ipv4 "$host_ip" || fail "PICO_TAP_HOST must be a dotted-quad IPv4 address"
valid_ipv4 "$stack_ip" || fail "PICO_TAP_STACK must be a dotted-quad IPv4 address"
valid_ipv4 "$netmask" || fail "PICO_TAP_NETMASK must be a dotted-quad IPv4 netmask"
prefix=$(mask_prefix) || fail "PICO_TAP_NETMASK must be a contiguous IPv4 netmask from /1 through /30"
host_value=$(ipv4_to_int "$host_ip")
stack_value=$(ipv4_to_int "$stack_ip")
mask_value=$(( (0xffffffff << (32 - prefix)) & 0xffffffff ))
network_value=$(( host_value & mask_value ))
broadcast_value=$(( network_value | (~mask_value & 0xffffffff) ))
is_private_ipv4() {
  (( ($1 & 0xff000000) == 0x0a000000 ||
     ($1 & 0xfff00000) == 0xac100000 ||
     ($1 & 0xffff0000) == 0xc0a80000 ))
}
(( host_value != stack_value &&
   (host_value & mask_value) == (stack_value & mask_value) &&
   host_value > network_value && host_value < broadcast_value &&
   stack_value > network_value && stack_value < broadcast_value )) ||
  fail "PICO_TAP_HOST and PICO_TAP_STACK must be distinct addresses on the same subnet"
if ! is_private_ipv4 "$host_value" || ! is_private_ipv4 "$stack_value"; then
  fail "PICO_TAP_HOST and PICO_TAP_STACK must use RFC 1918 private addresses"
fi
if ip link show dev "$tap_name" >/dev/null 2>&1; then
  fail "interface $tap_name already exists; choose another PICO_TAP_NAME"
fi

ip tuntap add dev "$tap_name" mode tap || fail "could not create TAP interface; check CAP_NET_ADMIN and /dev/net/tun permissions"
tap_created=1
ip address add "$host_ip/$prefix" dev "$tap_name" ||
  fail "could not assign $host_ip/$netmask to $tap_name"
ip link set dev "$tap_name" up || fail "could not bring $tap_name up"

"$picoapp" --tap "$tap_name:$stack_ip:$netmask" --app noop &
stack_pid=$!

for (( attempt = 0; attempt < 10; attempt++ )); do
  if ! kill -0 "$stack_pid" 2>/dev/null; then
    wait "$stack_pid" || fail "picoapp exited before the IPv4 packet-path check"
    fail "picoapp exited before the IPv4 packet-path check"
  fi
  if ping_output=$(ping -4 -n -q -c 1 -W 1 -I "$tap_name" "$stack_ip" 2>&1); then
    cleanup || fail "unable to clean up TAP interface $tap_name"
    printf 'PASS: ICMPv4 echo reply received from %s over TAP %s; interface removed\n' "$stack_ip" "$tap_name"
    exit 0
  elif [[ $ping_output == *"Operation not permitted"* || $ping_output == *"Permission denied"* ]]; then
    fail "ping lacks CAP_NET_RAW or permission to use ICMP sockets"
  fi
done

fail "no ICMPv4 echo reply from $stack_ip over TAP $tap_name within 10 seconds"
