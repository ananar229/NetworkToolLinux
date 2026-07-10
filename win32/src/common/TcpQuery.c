#include "TcpQuery.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <winsock2.h>
#include <ws2tcpip.h>

BOOL TcpQuery_SendReceive(const wchar_t *host, unsigned short port, const wchar_t *requestLine, wchar_t **outResponse,
                           wchar_t *errorMsg, int errorMsgSize) {
    *outResponse = NULL;

    wchar_t portStr[8];
    swprintf(portStr, 8, L"%u", port);

    ADDRINFOW hints;
    ZeroMemory(&hints, sizeof(hints));
    hints.ai_family = AF_INET;
    hints.ai_socktype = SOCK_STREAM;

    ADDRINFOW *result = NULL;
    if (GetAddrInfoW(host, portStr, &hints, &result) != 0 || !result) {
        _snwprintf(errorMsg, errorMsgSize, L"Could not resolve host: %s", host);
        return FALSE;
    }

    SOCKET sock = socket(result->ai_family, result->ai_socktype, result->ai_protocol);
    if (sock == INVALID_SOCKET) {
        FreeAddrInfoW(result);
        _snwprintf(errorMsg, errorMsgSize, L"Could not create socket.");
        return FALSE;
    }

    DWORD timeout = 10000;
    setsockopt(sock, SOL_SOCKET, SO_RCVTIMEO, (const char *)&timeout, sizeof(timeout));
    setsockopt(sock, SOL_SOCKET, SO_SNDTIMEO, (const char *)&timeout, sizeof(timeout));

    if (connect(sock, result->ai_addr, (int)result->ai_addrlen) != 0) {
        closesocket(sock);
        FreeAddrInfoW(result);
        _snwprintf(errorMsg, errorMsgSize, L"Connection to %s:%u failed.", host, port);
        return FALSE;
    }
    FreeAddrInfoW(result);

    char sendBuf[512];
    int sendLen = WideCharToMultiByte(CP_ACP, 0, requestLine, -1, sendBuf, (int)sizeof(sendBuf) - 2, NULL, NULL);
    if (sendLen > 0) {
        sendBuf[sendLen - 1] = '\r';
        sendBuf[sendLen] = '\n';
        sendLen++;
    }
    send(sock, sendBuf, sendLen, 0);

    size_t capacity = 8192;
    size_t used = 0;
    char *buffer = (char *)malloc(capacity);
    char chunk[4096];
    int received;
    while ((received = recv(sock, chunk, sizeof(chunk), 0)) > 0) {
        if (used + (size_t)received + 1 > capacity) {
            capacity *= 2;
            buffer = (char *)realloc(buffer, capacity);
        }
        memcpy(buffer + used, chunk, received);
        used += received;
    }
    buffer[used] = '\0';
    closesocket(sock);

    int wideLen = MultiByteToWideChar(CP_ACP, 0, buffer, (int)used + 1, NULL, 0);
    wchar_t *wide = (wchar_t *)malloc(wideLen * sizeof(wchar_t));
    MultiByteToWideChar(CP_ACP, 0, buffer, (int)used + 1, wide, wideLen);
    free(buffer);

    *outResponse = wide;
    return TRUE;
}
