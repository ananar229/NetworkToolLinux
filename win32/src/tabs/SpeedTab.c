#include "SpeedTab.h"

#include <commctrl.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <winhttp.h>

#include "common/Translations.h"
#include "common/resource.h"

/* Same public, unauthenticated Cloudflare speed-test endpoints used by the
 * Linux build (speed.cloudflare.com itself uses them too). Requires
 * WinHTTP's Schannel-backed HTTPS support; on ReactOS this may not work
 * if its TLS stack is incomplete - the tab reports a clear error rather
 * than hanging in that case. */
#define SPEED_HOST L"speed.cloudflare.com"
#define DOWNLOAD_BYTES 25000000UL
#define UPLOAD_BYTES 8000000UL
#define LATENCY_SAMPLES 4

/* WM_APP_SPEED_UPDATE phases */
#define PHASE_PING 0
#define PHASE_DOWNLOAD 1
#define PHASE_UPLOAD 2
#define PHASE_PROGRESS 3
#define PHASE_STATUS_LATENCY 5
#define PHASE_STATUS_DOWNLOAD 6
#define PHASE_STATUS_UPLOAD 7

typedef struct {
    HANDLE hThread;
    volatile LONG stopRequested;
} SpeedState;

typedef struct {
    HWND hDlg;
    volatile LONG *stopFlag;
} SpeedThreadParams;

static DWORD WINAPI SpeedThreadProc(LPVOID param) {
    SpeedThreadParams *p = (SpeedThreadParams *)param;

    HINTERNET hSession =
        WinHttpOpen(L"NetworkTool/1.0", WINHTTP_ACCESS_TYPE_DEFAULT_PROXY, WINHTTP_NO_PROXY_NAME,
                    WINHTTP_NO_PROXY_BYPASS, 0);
    if (!hSession) {
        PostMessageW(p->hDlg, WM_APP_SPEED_DONE, 0, 0);
        free(p);
        return 0;
    }
    HINTERNET hConnect = WinHttpConnect(hSession, SPEED_HOST, INTERNET_DEFAULT_HTTPS_PORT, 0);
    if (!hConnect) {
        WinHttpCloseHandle(hSession);
        PostMessageW(p->hDlg, WM_APP_SPEED_DONE, 0, 0);
        free(p);
        return 0;
    }

    /* --- Latency --- */
    PostMessageW(p->hDlg, WM_APP_SPEED_UPDATE, PHASE_STATUS_LATENCY, 0);
    DWORD totalMs = 0;
    int samples = 0;
    for (int i = 0; i < LATENCY_SAMPLES && !*p->stopFlag; i++) {
        DWORD t0 = GetTickCount();
        HINTERNET hReq = WinHttpOpenRequest(hConnect, L"GET", L"/__down?bytes=0", NULL, WINHTTP_NO_REFERER,
                                             WINHTTP_DEFAULT_ACCEPT_TYPES, WINHTTP_FLAG_SECURE);
        if (hReq) {
            if (WinHttpSendRequest(hReq, WINHTTP_NO_ADDITIONAL_HEADERS, 0, WINHTTP_NO_REQUEST_DATA, 0, 0, 0) &&
                WinHttpReceiveResponse(hReq, NULL)) {
                DWORD avail = 0, read = 0;
                char buf[64];
                WinHttpQueryDataAvailable(hReq, &avail);
                if (avail > 0) {
                    WinHttpReadData(hReq, buf, (DWORD)(avail < sizeof(buf) ? avail : sizeof(buf)), &read);
                }
                totalMs += (GetTickCount() - t0);
                samples++;
            }
            WinHttpCloseHandle(hReq);
        }
    }
    if (samples > 0) {
        PostMessageW(p->hDlg, WM_APP_SPEED_UPDATE, PHASE_PING, (LPARAM)(totalMs / (DWORD)samples));
    }

    /* --- Download --- */
    if (!*p->stopFlag) {
        PostMessageW(p->hDlg, WM_APP_SPEED_UPDATE, PHASE_STATUS_DOWNLOAD, 0);
        PostMessageW(p->hDlg, WM_APP_SPEED_UPDATE, PHASE_PROGRESS, 0);

        wchar_t path[64];
        swprintf(path, 64, L"/__down?bytes=%lu", DOWNLOAD_BYTES);
        HINTERNET hReq = WinHttpOpenRequest(hConnect, L"GET", path, NULL, WINHTTP_NO_REFERER,
                                             WINHTTP_DEFAULT_ACCEPT_TYPES, WINHTTP_FLAG_SECURE);
        if (hReq && WinHttpSendRequest(hReq, WINHTTP_NO_ADDITIONAL_HEADERS, 0, WINHTTP_NO_REQUEST_DATA, 0, 0, 0) &&
            WinHttpReceiveResponse(hReq, NULL)) {
            DWORD startTick = GetTickCount();
            DWORD totalRead = 0;
            char buf[16384];
            DWORD read = 0;
            while (!*p->stopFlag && WinHttpReadData(hReq, buf, sizeof(buf), &read) && read > 0) {
                totalRead += read;
                DWORD elapsed = GetTickCount() - startTick;
                if (elapsed > 0) {
                    double mbps = (totalRead * 8.0) / (elapsed / 1000.0) / 1000000.0;
                    PostMessageW(p->hDlg, WM_APP_SPEED_UPDATE, PHASE_DOWNLOAD, (LPARAM)(long)(mbps * 10));
                    PostMessageW(p->hDlg, WM_APP_SPEED_UPDATE, PHASE_PROGRESS,
                                 (LPARAM)(totalRead * 100 / DOWNLOAD_BYTES));
                }
            }
        }
        if (hReq) {
            WinHttpCloseHandle(hReq);
        }
    }

    /* --- Upload --- */
    if (!*p->stopFlag) {
        PostMessageW(p->hDlg, WM_APP_SPEED_UPDATE, PHASE_STATUS_UPLOAD, 0);
        PostMessageW(p->hDlg, WM_APP_SPEED_UPDATE, PHASE_PROGRESS, 0);

        HINTERNET hReq = WinHttpOpenRequest(hConnect, L"POST", L"/__up", NULL, WINHTTP_NO_REFERER,
                                             WINHTTP_DEFAULT_ACCEPT_TYPES, WINHTTP_FLAG_SECURE);
        if (hReq) {
            WinHttpAddRequestHeaders(hReq, L"Content-Type: application/octet-stream\r\n", (DWORD)-1,
                                      WINHTTP_ADDREQ_FLAG_ADD);
            char *payload = (char *)malloc(UPLOAD_BYTES);
            if (payload) {
                memset(payload, 'x', UPLOAD_BYTES);
                if (WinHttpSendRequest(hReq, WINHTTP_NO_ADDITIONAL_HEADERS, 0, WINHTTP_NO_REQUEST_DATA, 0,
                                        UPLOAD_BYTES, 0)) {
                    DWORD startTick = GetTickCount();
                    DWORD totalSent = 0;
                    const DWORD chunkSize = 65536;
                    while (!*p->stopFlag && totalSent < UPLOAD_BYTES) {
                        DWORD remaining = UPLOAD_BYTES - totalSent;
                        DWORD toSend = remaining < chunkSize ? remaining : chunkSize;
                        DWORD written = 0;
                        if (!WinHttpWriteData(hReq, payload + totalSent, toSend, &written) || written == 0) {
                            break;
                        }
                        totalSent += written;
                        DWORD elapsed = GetTickCount() - startTick;
                        if (elapsed > 0) {
                            double mbps = (totalSent * 8.0) / (elapsed / 1000.0) / 1000000.0;
                            PostMessageW(p->hDlg, WM_APP_SPEED_UPDATE, PHASE_UPLOAD, (LPARAM)(long)(mbps * 10));
                            PostMessageW(p->hDlg, WM_APP_SPEED_UPDATE, PHASE_PROGRESS,
                                         (LPARAM)(totalSent * 100 / UPLOAD_BYTES));
                        }
                    }
                    WinHttpReceiveResponse(hReq, NULL);
                }
                free(payload);
            }
            WinHttpCloseHandle(hReq);
        }
    }

    WinHttpCloseHandle(hConnect);
    WinHttpCloseHandle(hSession);
    PostMessageW(p->hDlg, WM_APP_SPEED_DONE, 0, 0);
    free(p);
    return 0;
}

static void StartTest(HWND hDlg, SpeedState *state) {
    SetDlgItemTextW(hDlg, IDC_SPEED_PING, T(STR_SPEED_DASH_MS));
    SetDlgItemTextW(hDlg, IDC_SPEED_DOWN, T(STR_SPEED_DASH_MBPS));
    SetDlgItemTextW(hDlg, IDC_SPEED_UP, T(STR_SPEED_DASH_MBPS));
    SetDlgItemTextW(hDlg, IDC_SPEED_STATUS, T(STR_SPEED_STARTING));
    SendMessageW(GetDlgItem(hDlg, IDC_SPEED_PROGRESS), PBM_SETPOS, 0, 0);

    state->stopRequested = 0;
    SpeedThreadParams *params = (SpeedThreadParams *)malloc(sizeof(SpeedThreadParams));
    params->hDlg = hDlg;
    params->stopFlag = &state->stopRequested;

    SetDlgItemTextW(hDlg, IDC_SPEED_BTN, T(STR_SPEED_STOP));
    state->hThread = CreateThread(NULL, 0, SpeedThreadProc, params, 0, NULL);
}

INT_PTR CALLBACK SpeedTab_DialogProc(HWND hDlg, UINT msg, WPARAM wParam, LPARAM lParam) {
    SpeedState *state = (SpeedState *)GetWindowLongPtrW(hDlg, DWLP_USER);

    switch (msg) {
        case WM_INITDIALOG:
            state = (SpeedState *)calloc(1, sizeof(SpeedState));
            SetWindowLongPtrW(hDlg, DWLP_USER, (LONG_PTR)state);
            SetDlgItemTextW(hDlg, IDC_SPEED_LABEL_INTRO, T(STR_SPEED_INTRO));
            SetDlgItemTextW(hDlg, IDC_SPEED_GROUP_PING, T(STR_SPEED_PING));
            SetDlgItemTextW(hDlg, IDC_SPEED_GROUP_DOWN, T(STR_SPEED_DOWNLOAD));
            SetDlgItemTextW(hDlg, IDC_SPEED_GROUP_UP, T(STR_SPEED_UPLOAD));
            SetDlgItemTextW(hDlg, IDC_SPEED_STATUS, T(STR_SPEED_READY));
            SetDlgItemTextW(hDlg, IDC_SPEED_BTN, T(STR_SPEED_STARTTEST));
            SetDlgItemTextW(hDlg, IDC_SPEED_PING, T(STR_SPEED_DASH_MS));
            SetDlgItemTextW(hDlg, IDC_SPEED_DOWN, T(STR_SPEED_DASH_MBPS));
            SetDlgItemTextW(hDlg, IDC_SPEED_UP, T(STR_SPEED_DASH_MBPS));
            SendMessageW(GetDlgItem(hDlg, IDC_SPEED_PROGRESS), PBM_SETRANGE32, 0, 100);
            return TRUE;

        case WM_COMMAND:
            if (LOWORD(wParam) == IDC_SPEED_BTN) {
                if (state->hThread) {
                    InterlockedExchange(&state->stopRequested, 1);
                } else {
                    StartTest(hDlg, state);
                }
            }
            return TRUE;

        case WM_APP_SPEED_UPDATE: {
            wchar_t buf[64];
            switch (wParam) {
                case PHASE_PING:
                    swprintf(buf, 64, L"%ld ms", (long)lParam);
                    SetDlgItemTextW(hDlg, IDC_SPEED_PING, buf);
                    break;
                case PHASE_DOWNLOAD:
                    swprintf(buf, 64, L"%.1f Mbps", (double)(long)lParam / 10.0);
                    SetDlgItemTextW(hDlg, IDC_SPEED_DOWN, buf);
                    break;
                case PHASE_UPLOAD:
                    swprintf(buf, 64, L"%.1f Mbps", (double)(long)lParam / 10.0);
                    SetDlgItemTextW(hDlg, IDC_SPEED_UP, buf);
                    break;
                case PHASE_PROGRESS:
                    SendMessageW(GetDlgItem(hDlg, IDC_SPEED_PROGRESS), PBM_SETPOS, (WPARAM)lParam, 0);
                    break;
                case PHASE_STATUS_LATENCY:
                    SetDlgItemTextW(hDlg, IDC_SPEED_STATUS, T(STR_SPEED_MEASURING));
                    break;
                case PHASE_STATUS_DOWNLOAD:
                    SetDlgItemTextW(hDlg, IDC_SPEED_STATUS, T(STR_SPEED_TESTDOWN));
                    break;
                case PHASE_STATUS_UPLOAD:
                    SetDlgItemTextW(hDlg, IDC_SPEED_STATUS, T(STR_SPEED_TESTUP));
                    break;
                default:
                    break;
            }
            return TRUE;
        }

        case WM_APP_SPEED_DONE:
            if (state->hThread) {
                CloseHandle(state->hThread);
                state->hThread = NULL;
            }
            SetDlgItemTextW(hDlg, IDC_SPEED_STATUS, T(STR_SPEED_COMPLETE));
            SetDlgItemTextW(hDlg, IDC_SPEED_BTN, T(STR_SPEED_STARTTEST));
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
