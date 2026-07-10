#include "TracerouteTab.h"

#include <ipexport.h>
#include <icmpapi.h>
#include <stdio.h>
#include <stdlib.h>
#include <wchar.h>
#include <winsock2.h>
#include <ws2tcpip.h>

#include "common/OutputEdit.h"
#include "common/ResolveIPv4.h"
#include "common/Translations.h"
#include "common/resource.h"

#define MAX_HOPS 64

typedef struct {
    HANDLE hThread;
    volatile LONG stopRequested;
} TraceState;

typedef struct {
    HWND hDlg;
    wchar_t address[256];
    volatile LONG *stopFlag;
} TraceThreadParams;

static void PostLine(HWND hDlg, UINT msg, const wchar_t *text) {
    size_t len = wcslen(text) + 1;
    wchar_t *copy = (wchar_t *)malloc(len * sizeof(wchar_t));
    if (!copy) {
        return;
    }
    wcscpy(copy, text);
    PostMessageW(hDlg, msg, 0, (LPARAM)copy);
}

static DWORD WINAPI TraceThreadProc(LPVOID param) {
    TraceThreadParams *p = (TraceThreadParams *)param;
    wchar_t line[256];

    DWORD destAddr;
    if (!ResolveIPv4(p->address, &destAddr)) {
        swprintf(line, 256, T(STR_TRACE_NORESOLVE), p->address);
        wcscat(line, L"\r\n");
        PostLine(p->hDlg, WM_APP_TEXT_LINE, line);
        PostMessageW(p->hDlg, WM_APP_TRACE_DONE, 0, 0);
        free(p);
        return 0;
    }

    HANDLE hIcmp = IcmpCreateFile();
    if (hIcmp == INVALID_HANDLE_VALUE) {
        swprintf(line, 256, L"%s\r\n", T(STR_TRACE_NOICMP));
        PostLine(p->hDlg, WM_APP_TEXT_LINE, line);
        PostMessageW(p->hDlg, WM_APP_TRACE_DONE, 0, 0);
        free(p);
        return 0;
    }

    char sendData[32] = "NetworkToolTracePayload";
    DWORD replySize = sizeof(ICMP_ECHO_REPLY) + sizeof(sendData) + 8;
    LPVOID replyBuffer = malloc(replySize);

    IP_OPTION_INFORMATION options;
    ZeroMemory(&options, sizeof(options));
    options.Ttl = 1;

    for (int ttl = 1; ttl <= MAX_HOPS && !*p->stopFlag; ttl++) {
        options.Ttl = (UCHAR)ttl;
        DWORD ret =
            IcmpSendEcho(hIcmp, destAddr, sendData, (WORD)sizeof(sendData), &options, replyBuffer, replySize, 3000);

        if (ret > 0) {
            PICMP_ECHO_REPLY reply = (PICMP_ECHO_REPLY)replyBuffer;
            struct in_addr a;
            a.S_un.S_addr = reply->Address;
            wchar_t addrText[32];
            MultiByteToWideChar(CP_ACP, 0, inet_ntoa(a), -1, addrText, 32);
            swprintf(line, 256, L"%2d  %s  %lums\r\n", ttl, addrText, (unsigned long)reply->RoundTripTime);
            PostLine(p->hDlg, WM_APP_TEXT_LINE, line);

            if (reply->Status == IP_SUCCESS) {
                PostMessageW(p->hDlg, WM_APP_TRACE_DONE, 1, 0);
                free(replyBuffer);
                IcmpCloseHandle(hIcmp);
                free(p);
                return 0;
            }
        } else {
            swprintf(line, 256, L"%2d  *\r\n", ttl);
            PostLine(p->hDlg, WM_APP_TEXT_LINE, line);
        }
    }

    free(replyBuffer);
    IcmpCloseHandle(hIcmp);
    PostMessageW(p->hDlg, WM_APP_TRACE_DONE, 0, 0);
    free(p);
    return 0;
}

static void StartTrace(HWND hDlg, TraceState *state) {
    wchar_t address[256];
    GetDlgItemTextW(hDlg, IDC_TRACE_ADDR, address, 256);
    HWND output = GetDlgItem(hDlg, IDC_TRACE_OUTPUT);
    if (wcslen(address) == 0) {
        OutputEdit_AppendLine(output, T(STR_TRACE_NOADDR));
        return;
    }

    OutputEdit_Clear(output);
    wchar_t header[300];
    swprintf(header, 300, T(STR_TRACE_HEADER), address);
    wcscat(header, L"\r\n");
    OutputEdit_Append(output, header);

    state->stopRequested = 0;

    TraceThreadParams *params = (TraceThreadParams *)malloc(sizeof(TraceThreadParams));
    params->hDlg = hDlg;
    wcscpy(params->address, address);
    params->stopFlag = &state->stopRequested;

    SetDlgItemTextW(hDlg, IDC_TRACE_BTN, T(STR_TRACE_STOP));
    EnableWindow(GetDlgItem(hDlg, IDC_TRACE_ADDR), FALSE);
    state->hThread = CreateThread(NULL, 0, TraceThreadProc, params, 0, NULL);
}

INT_PTR CALLBACK TracerouteTab_DialogProc(HWND hDlg, UINT msg, WPARAM wParam, LPARAM lParam) {
    TraceState *state = (TraceState *)GetWindowLongPtrW(hDlg, DWLP_USER);

    switch (msg) {
        case WM_INITDIALOG:
            state = (TraceState *)calloc(1, sizeof(TraceState));
            SetWindowLongPtrW(hDlg, DWLP_USER, (LONG_PTR)state);
            SetDlgItemTextW(hDlg, IDC_TRACE_LABEL_ADDR, T(STR_TRACE_LABEL));
            SetDlgItemTextW(hDlg, IDC_TRACE_BTN, T(STR_TRACE_BTN));
            return TRUE;

        case WM_COMMAND:
            if (LOWORD(wParam) == IDC_TRACE_BTN) {
                if (state->hThread) {
                    InterlockedExchange(&state->stopRequested, 1);
                } else {
                    StartTrace(hDlg, state);
                }
            }
            return TRUE;

        case WM_APP_TEXT_LINE: {
            wchar_t *text = (wchar_t *)lParam;
            OutputEdit_Append(GetDlgItem(hDlg, IDC_TRACE_OUTPUT), text);
            free(text);
            return TRUE;
        }

        case WM_APP_TRACE_DONE:
            if (state->hThread) {
                CloseHandle(state->hThread);
                state->hThread = NULL;
            }
            if (wParam == 1) {
                wchar_t reached[128];
                swprintf(reached, 128, L"\r\n%s", T(STR_TRACE_DESTREACHED));
                OutputEdit_AppendLine(GetDlgItem(hDlg, IDC_TRACE_OUTPUT), reached);
            }
            SetDlgItemTextW(hDlg, IDC_TRACE_BTN, T(STR_TRACE_BTN));
            EnableWindow(GetDlgItem(hDlg, IDC_TRACE_ADDR), TRUE);
            return TRUE;

        case WM_DESTROY:
            if (state) {
                if (state->hThread) {
                    InterlockedExchange(&state->stopRequested, 1);
                    WaitForSingleObject(state->hThread, 4000);
                    CloseHandle(state->hThread);
                }
                free(state);
                SetWindowLongPtrW(hDlg, DWLP_USER, 0);
            }
            return TRUE;

        default:
            break;
    }
    return FALSE;
}
