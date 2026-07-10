#pragma once

#include <windows.h>

/* Connects to host:port, sends requestLine + CRLF, reads the response
 * until the peer closes the connection (used by Whois/Finger, both
 * simple "one line in, text back" TCP protocols). On success *outResponse
 * is a malloc'd, NUL-terminated wide string the caller must free.
 * On failure returns FALSE and writes a message into errorMsg. */
BOOL TcpQuery_SendReceive(const wchar_t *host, unsigned short port, const wchar_t *requestLine, wchar_t **outResponse,
                           wchar_t *errorMsg, int errorMsgSize);
