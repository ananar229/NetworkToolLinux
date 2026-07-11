#include "PingTab.h"

#include <ipexport.h>
#include <icmpapi.h>
#include <stdio.h>
#include <stdlib.h>
#include <wchar.h>
#include <winsock2.h>
#include <ws2tcpip.h>

#include "common/DialogResize.h"
#include "common/OutputEdit.h"
#include "common/ResolveIPv4.h"
#include "common/Translations.h"
#include "common/resource.h"

typedef struct {
    HANDLE hThread;
    volatile LONG stopRequested;
    DialogResizer *resizer;
} PingState;

static const ResizeAnchor kAnchors[] = {
    {IDC_PING_ADDR, RESIZE_LEFT | RESIZE_TOP | RESIZE_RIGHT},
    {IDC_PING_BTN, RESIZE_RIGHT | RESIZE_TOP},
    {IDC_PING_OUTPUT, RESIZE_LEFT | RESIZE_TOP | RESIZE_RIGHT | RESIZE_BOTTOM},
};

typedef struct {
    HWND hDlg;
    wchar_t address[256];
    BOOL limited;
    int count;
    volatile LONG *stopFlag;
} PingThreadParams;

static void PostLine(HWND hDlg, UINT msg, const wchar_t *text) {
    size_t len = wcslen(text) + 1;
    wchar_t *copy = (wchar_t *)malloc(len * sizeof(wchar_t));
    if (!copy) {
        return;
    }
    wcscpy(copy, text);
    PostMessageW(hDlg, msg, 0, (LPARAM)copy);
}

static DWORD WINAPI PingThreadProc(LPVOID param) {
    PingThreadParams *p = (PingThreadParams *)param;
    wchar_t line[256];

    DWORD destAddr;
    if (!ResolveIPv4(p->address, &destAddr)) {
        swprintf(line, 256, T(STR_PING_NOTFOUND), p->address);
        wcscat(line, L"\r\n");
        PostLine(p->hDlg, WM_APP_PING_LINE, line);
        PostMessageW(p->hDlg, WM_APP_PING_DONE, 0, 0);
        free(p);
        return 0;
    }

    HANDLE hIcmp = IcmpCreateFile();
    if (hIcmp == INVALID_HANDLE_VALUE) {
        swprintf(line, 256, L"%s\r\n", T(STR_PING_NOICMP));
        PostLine(p->hDlg, WM_APP_PING_LINE, line);
        PostMessageW(p->hDlg, WM_APP_PING_DONE, 0, 0);
        free(p);
        return 0;
    }

    char sendData[32] = "NetworkToolPingPayload";
    DWORD replySize = sizeof(ICMP_ECHO_REPLY) + sizeof(sendData) + 8;
    LPVOID replyBuffer = malloc(replySize);

    int seq = 0;
    while (!*p->stopFlag && (!p->limited || seq < p->count)) {
        seq++;
        DWORD ret = IcmpSendEcho(hIcmp, destAddr, sendData, (WORD)sizeof(sendData), NULL, replyBuffer, replySize, 4000);
        if (ret > 0) {
            PICMP_ECHO_REPLY reply = (PICMP_ECHO_REPLY)replyBuffer;
            struct in_addr a;
            a.S_un.S_addr = reply->Address;
            wchar_t addrText[32];
            MultiByteToWideChar(CP_ACP, 0, inet_ntoa(a), -1, addrText, 32);
            swprintf(line, 256, T(STR_PING_REPLY), addrText, (unsigned long)reply->DataSize,
                     (unsigned long)reply->RoundTripTime, reply->Options.Ttl);
            wcscat(line, L"\r\n");
        } else {
            DWORD err = GetLastError();
            if (err == IP_REQ_TIMED_OUT) {
                swprintf(line, 256, L"%s\r\n", T(STR_PING_TIMEDOUT));
            } else {
                swprintf(line, 256, T(STR_PING_FAILED), (unsigned long)err);
                wcscat(line, L"\r\n");
            }
        }
        PostLine(p->hDlg, WM_APP_PING_LINE, line);

        if (!*p->stopFlag && (!p->limited || seq < p->count)) {
            Sleep(1000);
        }
    }

    free(replyBuffer);
    IcmpCloseHandle(hIcmp);
    PostMessageW(p->hDlg, WM_APP_PING_DONE, 0, 0);
    free(p);
    return 0;
}

static void StartPing(HWND hDlg, PingState *state) {
    wchar_t address[256];
    GetDlgItemTextW(hDlg, IDC_PING_ADDR, address, 256);
    if (wcslen(address) == 0) {
        OutputEdit_AppendLine(GetDlgItem(hDlg, IDC_PING_OUTPUT), T(STR_PING_NOADDR));
        return;
    }

    OutputEdit_Clear(GetDlgItem(hDlg, IDC_PING_OUTPUT));
    state->stopRequested = 0;

    PingThreadParams *params = (PingThreadParams *)malloc(sizeof(PingThreadParams));
    params->hDlg = hDlg;
    wcscpy(params->address, address);
    params->limited = IsDlgButtonChecked(hDlg, IDC_PING_LIMIT_CHK) == BST_CHECKED;
    params->count = GetDlgItemInt(hDlg, IDC_PING_COUNT, NULL, FALSE);
    if (params->count <= 0) {
        params->count = 10;
    }
    params->stopFlag = &state->stopRequested;

    SetDlgItemTextW(hDlg, IDC_PING_BTN, T(STR_PING_STOP));
    EnableWindow(GetDlgItem(hDlg, IDC_PING_ADDR), FALSE);
    state->hThread = CreateThread(NULL, 0, PingThreadProc, params, 0, NULL);
}

INT_PTR CALLBACK PingTab_DialogProc(HWND hDlg, UINT msg, WPARAM wParam, LPARAM lParam) {
    PingState *state = (PingState *)GetWindowLongPtrW(hDlg, DWLP_USER);

    switch (msg) {
        case WM_INITDIALOG:
            state = (PingState *)calloc(1, sizeof(PingState));
            SetWindowLongPtrW(hDlg, DWLP_USER, (LONG_PTR)state);
            SetDlgItemTextW(hDlg, IDC_PING_LABEL_ADDR, T(STR_PING_LABEL));
            SetDlgItemTextW(hDlg, IDC_PING_BTN, T(STR_PING_BTN));
            SetDlgItemTextW(hDlg, IDC_PING_LIMIT_CHK, T(STR_PING_SENDONLY));
            SetDlgItemTextW(hDlg, IDC_PING_LABEL_PINGS, T(STR_PING_PINGS));
            SetDlgItemTextW(hDlg, IDC_PING_COUNT, L"10");
            state->resizer = DialogResize_Init(hDlg, kAnchors, (int)(sizeof(kAnchors) / sizeof(kAnchors[0])));
            return TRUE;

        case WM_SIZE:
            if (state) {
                DialogResize_Apply(state->resizer, LOWORD(lParam), HIWORD(lParam));
            }
            return TRUE;

        case WM_COMMAND:
            if (LOWORD(wParam) == IDC_PING_BTN) {
                if (state->hThread) {
                    InterlockedExchange(&state->stopRequested, 1);
                } else {
                    StartPing(hDlg, state);
                }
            }
            return TRUE;

        case WM_APP_PING_LINE: {
            wchar_t *text = (wchar_t *)lParam;
            OutputEdit_Append(GetDlgItem(hDlg, IDC_PING_OUTPUT), text);
            free(text);
            return TRUE;
        }

        case WM_APP_PING_DONE:
            if (state->hThread) {
                CloseHandle(state->hThread);
                state->hThread = NULL;
            }
            SetDlgItemTextW(hDlg, IDC_PING_BTN, T(STR_PING_BTN));
            EnableWindow(GetDlgItem(hDlg, IDC_PING_ADDR), TRUE);
            return TRUE;

        case WM_DESTROY:
            if (state) {
                if (state->hThread) {
                    InterlockedExchange(&state->stopRequested, 1);
                    WaitForSingleObject(state->hThread, 2000);
                    CloseHandle(state->hThread);
                }
                DialogResize_Free(state->resizer);
                free(state);
                SetWindowLongPtrW(hDlg, DWLP_USER, 0);
            }
            return TRUE;

        default:
            break;
    }
    return FALSE;
}
