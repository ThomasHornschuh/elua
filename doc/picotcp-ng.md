# PicoTCP-NG provenance and Linux TAP validation

## Pinned source and license status

The `picotcp` submodule points to
[`virtualsquare/picotcp`](https://github.com/virtualsquare/picotcp) at commit
`d2f46200b0768bb1d08aff6ed575abeacf8cb557`, observed on 2026-11-08. This is
the upstream commit titled “Fix leaks in picoapp” (committed 2026-10-07).
Initialize it with:

```sh
git submodule update --init --recursive
git -C picotcp rev-parse HEAD
```

The upstream README declares GPLv2/v3-only; its source uses
`GPL-2.0-only OR GPL-3.0-only`. eLua's `LICENSE` describes eLua as MIT-licensed.
The relationship between those licenses for a combined eLua image and any
distribution obligations has not been resolved or approved by the project.
For this issue, the disposition is source checkout and separate host-only
validation only: do not bundle PicoTCP-NG into eLua firmware or distribute a
combined work. Any future distribution must retain upstream copyright and
license notices and satisfy the selected GPL version's terms, including its
source obligations. Do not describe PicoTCP-NG as MIT-licensed. No maintainer
compatibility approval is recorded; the dependent eLua adapter/firmware
integration must remain blocked until project maintainers explicitly accept a
compatibility and distribution decision.

## Build the upstream test application

On Linux, install GCC, GNU make, and the `libvdeplug` development package
(`libvdeplug2-dev` on Debian/Ubuntu). The upstream `make test` target
builds the upstream-compatible `test/picoapp.c` harness. From the repository
root, record the compiler version and build with TAP, IPv4/ICMP, and the
upstream test application enabled:

```sh
gcc --version | head -n 1
make -C picotcp PREFIX="$PWD/picotcp/build-host" clean
make -C picotcp \
  PREFIX="$PWD/picotcp/build-host" DEBUG=0 \
  TCP=1 UDP=1 ETH=1 IPV4=1 IPV4FRAG=1 ICMP4=1 PING=1 \
  IPV6=0 TAP=1 TUN=0 VDE=1 PCAP=0 TEST=1 \
  test
```

This uses only the upstream host build and does not use eLua's obsolete
`build_rv32*` archives. The selected variables are passed explicitly;
`TEST=1` is required by the upstream test target, and `VDE=1` plus
`libvdeplug` satisfy the upstream harness's link dependencies. The build output
is ignored by Git.

## Run the isolated TAP check

The host needs Linux `/dev/net/tun`, `iproute2`, `iputils-ping`, and sufficient
privileges: root/sudo with `CAP_NET_ADMIN` and `CAP_NET_RAW`. From the
repository root:

```sh
sudo ./tests/picotcp_tap.sh
```

The script creates a temporary TAP interface (default name `pico<PID>`) with
host address `192.168.254.1`, runs the PicoTCP stack at `192.168.254.2`, and
checks for an IPv4 ICMP echo reply within ten seconds. Success prints a `PASS:`
marker only after the interface has been removed. The script does not add a
gateway or change DNS, firewall policy, the default route, or a physical
interface. A trap terminates the upstream process and removes the interface on
failure or interruption too.

To select a different interface and static address pair, set
`PICO_TAP_NAME`, `PICO_TAP_HOST`, `PICO_TAP_STACK`, and `PICO_TAP_NETMASK`;
choose addresses on the same unused subnet. For example:

```sh
sudo env PICO_TAP_NAME=pico-test \
  PICO_TAP_HOST=192.168.253.1 PICO_TAP_STACK=192.168.253.2 \
  PICO_TAP_NETMASK=255.255.255.252 ./tests/picotcp_tap.sh
```

If `/dev/net/tun`, required commands, permissions, the built application, or
the packet-path check is unavailable, the script prints a `FAIL:` diagnostic
and exits nonzero. After any run, confirm cleanup with
`ip link show <interface-name>`; the interface should not exist.

## eLua integration status

The eLua adapter still targets the legacy PicoTCP API and archive layout. A
build selecting `tcpip.stack = "picotcp"` therefore stops before compilation
with a diagnostic pointing to the dependent Draft 2 integration work.
`board=bonfire_arty_minimal` is the network-free Bonfire alternative, and the
simulator remains a non-network build.
