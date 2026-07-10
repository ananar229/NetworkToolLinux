#include "ResolveIPv4.h"

#include <ws2tcpip.h>

BOOL ResolveIPv4(const wchar_t *host, DWORD *outAddr) {
    ADDRINFOW hints;
    ZeroMemory(&hints, sizeof(hints));
    hints.ai_family = AF_INET;
    hints.ai_socktype = SOCK_STREAM;

    ADDRINFOW *result = NULL;
    if (GetAddrInfoW(host, NULL, &hints, &result) != 0 || !result) {
        return FALSE;
    }

    struct sockaddr_in *addr = (struct sockaddr_in *)result->ai_addr;
    *outAddr = addr->sin_addr.S_un.S_addr;
    FreeAddrInfoW(result);
    return TRUE;
}
