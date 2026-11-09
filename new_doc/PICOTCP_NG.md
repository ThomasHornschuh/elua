# PicoTCP-NG on the Linux simulator

## Supported profile and licensing

`board=sim` remains network-free. The opt-in `board=sim_picotcp` profile builds
the native Linux simulator with the eLua `net` module and PicoTCP-NG TCP/UDP
support. The Lua builder fetches the source on demand into the ignored
`picotcp/` directory and checks out VirtualSquare PicoTCP-NG revision
`d2f46200b0768bb1d08aff6ed575abeacf8cb557`. It does not use a submodule,
developer-installed PicoTCP, or the old `build_rv32*` archives.

PicoTCP-NG is licensed `GPL-2.0-only OR GPL-3.0-only`; eLua's own license
remains separate. Every build that enables this integration prints a warning
to consider the applicable GPL terms before distributing binaries that include
PicoTCP-NG. Review the upstream license and obligations for your distribution.

## Build

The build host needs Linux, GCC, GNU make, Git, the Lua 5.1 build-script
runtime, and the same LuaRocks modules used by the regular simulator build.
Building the image does not open or create a TAP interface and does not require
network privileges:

```sh
lua5.1 build_elua.lua board=sim_picotcp lua_version=lua55 \
  build_mode=build_dir_linearized build_dir=.build-sim-picotcp \
  romfs_dir=tests/picotcp_romfs
```

The image is `elua_lua_sim_picotcp.elf`. The first build fetches the pinned
PicoTCP-NG revision; later builds reuse that checkout. To refresh a damaged
checkout, remove the ignored `picotcp/` directory and rebuild. The static
PicoTCP-NG library is built from that source for the native Linux target.

PicoTCP-NG is currently supported only for this native Linux profile. The
Bonfire Arty PicoTCP configuration and all other PicoTCP targets fail early
with an unsupported-profile diagnostic; the legacy Bonfire adapter is not
silently compiled against the new API. The profile uses static IPv4 and does not enable DHCP, DNS, mDNS, TFTP, or IPv6.
Synchronous `net.lookup` accepts numeric IPv4 addresses only; asynchronous DNS
lookup is unsupported and `net.nameserver` returns `-1`. `net.recv` supports a
byte count, not line mode. `net.debug` has no logging backend and specifying a
log file returns an unsupported-operation error. The existing TCP socket
options remain available through the Lua `net` constants and methods. The eLua
`net` socket, TCP, UDP, close, timer, and socket callback surface remains
available.

## Runtime TAP configuration

Linux TAP support requires `/dev/net/tun`, an existing TAP interface, and
permission to attach to it. Create and configure the TAP interface separately;
eLua never selects a physical interface, creates a bridge, adds a host default
route, or changes DNS. The interface name is required at runtime:

```sh
sudo ip tuntap add dev elua0 mode tap
sudo ip address add 192.168.254.1/30 dev elua0
sudo ip link set dev elua0 up
sudo env ELUA_PICO_TAP=elua0 ./elua_lua_sim_picotcp.elf
```

`ELUA_PICO_TAP` must name an existing Linux TAP interface and be 1–15 valid
interface-name characters. Missing `/dev/net/tun`, nonexistent/non-TAP
interfaces, invalid names, and attach errors are reported before the simulator
shell starts. The simulator attaches to that TAP and polls non-blockingly.

By default, the profile uses `192.168.254.2/24` and no gateway. Runtime
overrides are `ELUA_PICO_TAP_IP`, `ELUA_PICO_TAP_NETMASK`, and optional
`ELUA_PICO_TAP_GATEWAY`. These configure PicoTCP only; they do not change host
addresses, routes, or DNS. Use an isolated, unused subnet. Remove manually
created interfaces when finished:

```sh
sudo ip link delete dev elua0
```

PicoTCP polling and protocol timers run in the simulator's safe execution
context: a Lua VM instruction hook services the stack while Lua code runs,
blocking network operations poll while waiting, and the shell polls while idle.
The host timer signal does not call PicoTCP or execute Lua network callbacks.

## TCP over TAP integration check

Build the profile with `romfs_dir=tests/picotcp_romfs` as shown above. The
privileged helper creates one disposable TAP interface, configures only its
private point-to-point address, runs the Lua TCP server against a local Linux
peer, and removes the interface with a shell trap:

```sh
sudo env ELUA_SIM="$PWD/elua_lua_sim_picotcp.elf" \
  bash tests/picotcp_sim_tcp.sh
```

The helper needs Linux `iproute2`, Python 3, `/dev/net/tun`, and `CAP_NET_ADMIN`
(or equivalent root privileges). It does not change the default route or DNS,
and does not attach to a LAN or physical interface. It uses bounded connection
and process timeouts and prints `PASS:` only after the known TCP request and
response complete and the interface is removed. Ordinary simulator tests and
the unprivileged build do not invoke this helper or require TAP privileges.

The check establishes local TCP/IP connectivity only. It does not validate
Internet access, DHCP, DNS, hardware Ethernet, or Bonfire interoperability.

## SD-card network examples

`sim_picotcp` enables the same MMC/FAT component as `sim`, so it mounts a FAT
image named `sdcard.img` in its working directory at `/mmc`. Generated images
are ignored and must not be committed. Create a curated image from a checkout
of `elua_examples` with `mtools`:

```sh
ELUA_EXAMPLES_DIR=/path/to/elua_examples \
  tests/create_picotcp_examples_sd_image.sh /tmp/picotcp-sdcard.img
mkdir -p /tmp/picotcp-run
cp /tmp/picotcp-sdcard.img /tmp/picotcp-run/sdcard.img
cd /tmp/picotcp-run
sudo env ELUA_PICO_TAP=elua0 /path/to/elua/elua_lua_sim_picotcp.elf
```

The generator requires `truncate`, `mformat`, `mmd`, and `mcopy`; it refuses to
overwrite an image. It was checked with `elua_examples` revision
`06478dfde218897b0be38df3cf6e8eb2e2c55ffa`. The image includes
`/mmc/net/tcp_echo.lua`, `/mmc/net/http_get_text.lua`, and their local `util`
and `debugger` dependencies. `tcp_echo.lua` is the selected TCP-server example;
point a local TAP peer at port 7. `http_get_text.lua` is the selected
HTTP-style client; give it a numeric address and a local HTTP server on the
TAP subnet, for example `192.168.254.1:8080/`. Numeric addresses avoid the
intentionally unsupported DNS client.

### Compatibility matrix (`elua_examples/net` at the revision above)

| Script | Classification | Notes |
| --- | --- | --- |
| `tcp_echo.lua` | Runnable unchanged | Selected TAP TCP-server example; byte-count receive is used. |
| `http_get_text.lua` | Runnable with setup | Included on the SD image with `util.lua` and `debugger.lua`; use a numeric local HTTP URL. |
| `http_get_file.lua` | Runnable with setup | Needs `util.lua`, a numeric local HTTP endpoint, and writable `/mmc`; not included in the curated image. |
| `http_client.lua` | Runnable with adjustment | Use numeric local URLs; its `net.tick` calls are harmless because polling is automatic. |
| `http_get_text_old.lua` | Runnable with setup | Needs `util.lua` and `debugger.lua`, plus a numeric local HTTP endpoint. |
| `net_timer.lua` | Runnable with setup | Needs `debugger.lua`; timer callbacks and `net.tick` are supported. |
| `life_server.lua` | Runnable with adjustment | Use a local TAP peer; it uses line-oriented UI/protocol assumptions outside the TCP transport. |
| `ftpserver.lua` | Intentionally unsupported | Requires `net.get_driver_stats`, unavailable PicoTCP-NG buffered-driver statistics, and FTP peer setup. |
| `ftp.lua` | Intentionally unsupported | Depends on `ftpserver.lua` and unavailable driver statistics/debug logging. |
| `lookup.lua` | Intentionally unsupported | Requires DNS and debug/statistics helpers; this isolated profile has no DNS client. |
| `linkstat.lua` | Intentionally unsupported | Requires `util.lua` and legacy driver-link statistics with no PicoTCP-NG equivalent. |

`net.tick` remains exported for legacy programs. It explicitly services the
stack, but normal simulator operation also services PicoTCP while Lua executes
and while the shell is idle; applications do not need polling loops solely for
transport progress. Unsupported `net.lookup` hostname callbacks,
`net.nameserver`, debug-file logging, line-mode `net.recv`, and legacy driver
statistics fail predictably rather than using host DNS or a physical network.

## Portability boundary

The PicoTCP-NG wrapper uses a platform-neutral device-adapter interface. The
Linux implementation in `src/platform/sim/picotcp_tap.c` is the only code that
opens `/dev/net/tun`. A future Bonfire Arty adapter can provide the same
interface over its Ethernet driver without importing TAP code; it remains a
separate follow-up and is not enabled by this simulator profile.
