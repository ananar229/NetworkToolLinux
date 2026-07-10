#pragma once

#include <windows.h>

/* Resolves host (dotted IPv4 or hostname) to an IPv4 address in network
 * byte order. Returns FALSE if resolution fails or no IPv4 address exists. */
BOOL ResolveIPv4(const wchar_t *host, DWORD *outAddr);
