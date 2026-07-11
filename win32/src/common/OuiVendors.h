#pragma once

#include <stddef.h>

/* Best-effort MAC vendor lookup against a curated subset of the IEEE OUI
 * registry (see OuiVendors.c) - not exhaustive, returns NULL for unknown
 * prefixes. */
const wchar_t *OuiVendors_Lookup(const unsigned char mac[6]);
