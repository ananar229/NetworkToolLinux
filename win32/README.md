# Network Tool — Win32 / ReactOS port

A from-scratch reimplementation of the same nine tools (Info, Netstat, Ping,
Lookup, Traceroute, Whois, Finger, Port Scan, Speed) using plain Win32 API
and classic common controls — no Qt, no .NET. Targets `WINVER 0x0500`
(Windows 2000-era API surface) specifically so it also runs on ReactOS.

## How each tab talks to the network

| Tab | API |
|---|---|
| Info | `GetAdaptersInfo` + `GetIfEntry` (IP Helper) |
| Netstat | `GetIpForwardTable` / `GetTcpTable` / `GetUdpTable` / `Get{Ip,Tcp,Udp}Statistics` |
| Ping | `IcmpSendEcho` (unprivileged, no admin rights needed) |
| Lookup | `getaddrinfo` (default) / `DnsQuery_W` (specific record types) |
| Traceroute | `IcmpSendEcho` with increasing TTL — the same technique `tracert.exe` uses |
| Whois / Finger | Raw TCP via Winsock2 (ports 43 / 79), synchronous |
| Port Scan | 32-thread pool doing non-blocking `connect()` + `select()` |
| Speed | WinHTTP against the same Cloudflare endpoints the Linux build uses |

Ping, Traceroute, Port Scan and Speed run their network I/O on a worker
thread and post results back to the dialog via `WM_APP_*` messages, so the
UI never blocks. Lookup/Whois/Finger/Netstat are quick enough to run
synchronously.

## Building (cross-compile from Linux with MinGW-w64)

```sh
cmake -S . -B build -G Ninja -DCMAKE_TOOLCHAIN_FILE=cmake/mingw-toolchain.cmake
cmake --build build
```

Produces a single static-linked `build/NetworkTool.exe` (no MinGW runtime
DLLs needed alongside it).

On real Windows with MSVC, just open the folder in CMake-aware Visual
Studio / CLion — the toolchain file is only needed for cross-compiling.

## Known limitation: Speed tab under ReactOS

The Speed tab uses WinHTTP over HTTPS. ReactOS's Schannel/TLS stack has
historically lagged behind Windows, so that tab may fail there even though
everything else works — it reports a clear WinHTTP error rather than
hanging. All other tabs only need IP Helper / ICMP / plain TCP, which
ReactOS has supported for a long time.

## Testing notes

Verified: cross-compiles cleanly (zero warnings with `-Wall -Wextra`),
launches and runs stably under Wine on Linux, Info tab correctly reads and
live-refreshes real adapter data through IP Helper. Not verified: an actual
ReactOS installation, since none was available in this environment — Wine
approximates the Win32 API surface but isn't a substitute for testing on
ReactOS itself before relying on this.
