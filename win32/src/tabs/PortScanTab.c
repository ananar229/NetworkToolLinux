#include "PortScanTab.h"

#include <commctrl.h>
#include <stdio.h>
#include <stdlib.h>
#include <wchar.h>
#include <winsock2.h>
#include <ws2tcpip.h>

#include "common/OutputEdit.h"
#include "common/ResolveIPv4.h"
#include "common/Translations.h"
#include "common/resource.h"

#define SCAN_THREAD_COUNT 32
#define SCAN_TIMEOUT_MS 400

typedef struct {
    HWND hDlg;
    DWORD destAddr;
    volatile LONG nextPort;
    int endPort;
    int totalPorts;
    volatile LONG openCount;
    volatile LONG scannedCount;
    volatile LONG *stopFlag;
} ScanContext;

typedef struct {
    HANDLE hCoordinatorThread;
    volatile LONG stopRequested;
} PortScanState;

static BOOL TryConnect(DWORD addr, unsigned short port, DWORD timeoutMs) {
    SOCKET s = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
    if (s == INVALID_SOCKET) {
        return FALSE;
    }
    u_long mode = 1;
    ioctlsocket(s, FIONBIO, &mode);

    struct sockaddr_in sa;
    ZeroMemory(&sa, sizeof(sa));
    sa.sin_family = AF_INET;
    sa.sin_addr.S_un.S_addr = addr;
    sa.sin_port = htons(port);
    connect(s, (struct sockaddr *)&sa, sizeof(sa));

    fd_set writeSet, exceptSet;
    FD_ZERO(&writeSet);
    FD_SET(s, &writeSet);
    FD_ZERO(&exceptSet);
    FD_SET(s, &exceptSet);
    struct timeval tv;
    tv.tv_sec = timeoutMs / 1000;
    tv.tv_usec = (timeoutMs % 1000) * 1000;

    BOOL open = FALSE;
    if (select(0, NULL, &writeSet, &exceptSet, &tv) > 0 && FD_ISSET(s, &writeSet) && !FD_ISSET(s, &exceptSet)) {
        int err = 0;
        int len = sizeof(err);
        getsockopt(s, SOL_SOCKET, SO_ERROR, (char *)&err, &len);
        open = (err == 0);
    }
    closesocket(s);
    return open;
}

static DWORD WINAPI ScanWorker(LPVOID param) {
    ScanContext *ctx = (ScanContext *)param;
    for (;;) {
        if (*ctx->stopFlag) {
            break;
        }
        LONG port = InterlockedIncrement(&ctx->nextPort) - 1;
        if (port > ctx->endPort) {
            break;
        }
        BOOL open = TryConnect(ctx->destAddr, (unsigned short)port, SCAN_TIMEOUT_MS);
        LONG scanned = InterlockedIncrement(&ctx->scannedCount);
        if (open) {
            InterlockedIncrement(&ctx->openCount);
            PostMessageW(ctx->hDlg, WM_APP_SCAN_RESULT, (WPARAM)port, 0);
        }
        PostMessageW(ctx->hDlg, WM_APP_SCAN_PROGRESS, (WPARAM)scanned, (LPARAM)ctx->totalPorts);
    }
    return 0;
}

static DWORD WINAPI ScanCoordinator(LPVOID param) {
    ScanContext *ctx = (ScanContext *)param;

    HANDLE threads[SCAN_THREAD_COUNT];
    for (int i = 0; i < SCAN_THREAD_COUNT; i++) {
        threads[i] = CreateThread(NULL, 0, ScanWorker, ctx, 0, NULL);
    }
    WaitForMultipleObjects(SCAN_THREAD_COUNT, threads, TRUE, INFINITE);
    for (int i = 0; i < SCAN_THREAD_COUNT; i++) {
        CloseHandle(threads[i]);
    }

    PostMessageW(ctx->hDlg, WM_APP_SCAN_DONE, (WPARAM)ctx->openCount, (LPARAM)ctx->scannedCount);
    free(ctx);
    return 0;
}

static void StartScan(HWND hDlg, PortScanState *state) {
    wchar_t address[256];
    GetDlgItemTextW(hDlg, IDC_SCAN_ADDR, address, 256);
    HWND output = GetDlgItem(hDlg, IDC_SCAN_OUTPUT);
    OutputEdit_Clear(output);
    if (wcslen(address) == 0) {
        OutputEdit_AppendLine(output, T(STR_SCAN_NOADDR));
        return;
    }

    DWORD destAddr;
    if (!ResolveIPv4(address, &destAddr)) {
        OutputEdit_AppendLine(output, T(STR_SCAN_NORESOLVE));
        return;
    }

    int startPort = GetDlgItemInt(hDlg, IDC_SCAN_START, NULL, FALSE);
    int endPort = GetDlgItemInt(hDlg, IDC_SCAN_END, NULL, FALSE);
    if (startPort <= 0) {
        startPort = 1;
    }
    if (endPort <= 0) {
        endPort = 1024;
    }
    if (startPort > endPort) {
        int tmp = startPort;
        startPort = endPort;
        endPort = tmp;
    }

    wchar_t line[128];
    swprintf(line, 128, T(STR_SCAN_SCANNING), address, startPort, endPort);
    wcscat(line, L"\r\n");
    OutputEdit_Append(output, line);

    HWND progress = GetDlgItem(hDlg, IDC_SCAN_PROGRESS);
    SendMessageW(progress, PBM_SETRANGE32, 0, endPort - startPort + 1);
    SendMessageW(progress, PBM_SETPOS, 0, 0);

    state->stopRequested = 0;

    ScanContext *ctx = (ScanContext *)calloc(1, sizeof(ScanContext));
    ctx->hDlg = hDlg;
    ctx->destAddr = destAddr;
    ctx->nextPort = startPort;
    ctx->endPort = endPort;
    ctx->totalPorts = endPort - startPort + 1;
    ctx->stopFlag = &state->stopRequested;

    SetDlgItemTextW(hDlg, IDC_SCAN_BTN, T(STR_SCAN_STOP));
    EnableWindow(GetDlgItem(hDlg, IDC_SCAN_ADDR), FALSE);
    state->hCoordinatorThread = CreateThread(NULL, 0, ScanCoordinator, ctx, 0, NULL);
}

INT_PTR CALLBACK PortScanTab_DialogProc(HWND hDlg, UINT msg, WPARAM wParam, LPARAM lParam) {
    PortScanState *state = (PortScanState *)GetWindowLongPtrW(hDlg, DWLP_USER);

    switch (msg) {
        case WM_INITDIALOG:
            state = (PortScanState *)calloc(1, sizeof(PortScanState));
            SetWindowLongPtrW(hDlg, DWLP_USER, (LONG_PTR)state);
            SetDlgItemTextW(hDlg, IDC_SCAN_LABEL_ADDR, T(STR_SCAN_LABEL));
            SetDlgItemTextW(hDlg, IDC_SCAN_LABEL_BETWEEN, T(STR_SCAN_BETWEEN));
            SetDlgItemTextW(hDlg, IDC_SCAN_LABEL_AND, T(STR_SCAN_AND));
            SetDlgItemTextW(hDlg, IDC_SCAN_BTN, T(STR_SCAN_BTN));
            SetDlgItemTextW(hDlg, IDC_SCAN_WARNING, T(STR_SCAN_WARNING));
            SetDlgItemTextW(hDlg, IDC_SCAN_START, L"1");
            SetDlgItemTextW(hDlg, IDC_SCAN_END, L"1024");
            return TRUE;

        case WM_COMMAND:
            if (LOWORD(wParam) == IDC_SCAN_BTN) {
                if (state->hCoordinatorThread) {
                    InterlockedExchange(&state->stopRequested, 1);
                } else {
                    StartScan(hDlg, state);
                }
            }
            return TRUE;

        case WM_APP_SCAN_RESULT: {
            wchar_t line[64];
            swprintf(line, 64, T(STR_SCAN_OPENPORT), (int)wParam);
            OutputEdit_AppendLine(GetDlgItem(hDlg, IDC_SCAN_OUTPUT), line);
            return TRUE;
        }

        case WM_APP_SCAN_PROGRESS:
            SendMessageW(GetDlgItem(hDlg, IDC_SCAN_PROGRESS), PBM_SETPOS, wParam, 0);
            return TRUE;

        case WM_APP_SCAN_DONE: {
            if (state->hCoordinatorThread) {
                CloseHandle(state->hCoordinatorThread);
                state->hCoordinatorThread = NULL;
            }
            wchar_t line[160];
            wcscpy(line, L"\r\n");
            wchar_t finished[128];
            swprintf(finished, 128, T(STR_SCAN_FINISHED), (int)wParam, (int)lParam);
            wcscat(line, finished);
            OutputEdit_AppendLine(GetDlgItem(hDlg, IDC_SCAN_OUTPUT), line);
            SetDlgItemTextW(hDlg, IDC_SCAN_BTN, T(STR_SCAN_BTN));
            EnableWindow(GetDlgItem(hDlg, IDC_SCAN_ADDR), TRUE);
            return TRUE;
        }

        case WM_DESTROY:
            if (state) {
                if (state->hCoordinatorThread) {
                    InterlockedExchange(&state->stopRequested, 1);
                    WaitForSingleObject(state->hCoordinatorThread, 4000);
                    CloseHandle(state->hCoordinatorThread);
                }
                free(state);
                SetWindowLongPtrW(hDlg, DWLP_USER, 0);
            }
            return TRUE;

        case WM_CTLCOLORSTATIC:
            if (GetDlgCtrlID((HWND)lParam) == IDC_SCAN_WARNING) {
                HDC hdc = (HDC)wParam;
                SetTextColor(hdc, RGB(0xB0, 0x6A, 0x00));
                SetBkMode(hdc, TRANSPARENT);
                return (INT_PTR)GetSysColorBrush(COLOR_BTNFACE);
            }
            return FALSE;

        default:
            break;
    }
    (void)lParam;
    return FALSE;
}
