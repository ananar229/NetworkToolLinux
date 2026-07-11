#include "InfoTab.h"

#include <iphlpapi.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <wchar.h>
#include <windowsx.h>

#include "common/DialogResize.h"
#include "common/Translations.h"
#include "common/resource.h"

#define REFRESH_TIMER_ID 1

typedef struct {
    DialogResizer *resizer;
} InfoState;

static const ResizeAnchor kAnchors[] = {
    {IDC_INFO_LABEL_SELECT, RESIZE_LEFT | RESIZE_TOP},
    {IDC_INFO_IFACE_COMBO, RESIZE_LEFT | RESIZE_TOP | RESIZE_RIGHT},
    {IDC_INFO_GROUP_IFACE, RESIZE_LEFT | RESIZE_TOP},
    {IDC_INFO_GROUP_STATS, RESIZE_RIGHT | RESIZE_TOP},
    {IDC_INFO_LABEL_RXPACKETS, RESIZE_RIGHT | RESIZE_TOP},
    {IDC_INFO_RXPACKETS, RESIZE_RIGHT | RESIZE_TOP},
    {IDC_INFO_LABEL_RXERR, RESIZE_RIGHT | RESIZE_TOP},
    {IDC_INFO_RXERR, RESIZE_RIGHT | RESIZE_TOP},
    {IDC_INFO_LABEL_TXPACKETS, RESIZE_RIGHT | RESIZE_TOP},
    {IDC_INFO_TXPACKETS, RESIZE_RIGHT | RESIZE_TOP},
    {IDC_INFO_LABEL_TXERR, RESIZE_RIGHT | RESIZE_TOP},
    {IDC_INFO_TXERR, RESIZE_RIGHT | RESIZE_TOP},
};

static void AnsiToWide(const char *ansi, wchar_t *out, int outCount) {
    if (!ansi || !*ansi) {
        out[0] = L'\0';
        return;
    }
    MultiByteToWideChar(CP_ACP, 0, ansi, -1, out, outCount);
}

static PIP_ADAPTER_INFO FetchAdapters(void) {
    ULONG bufLen = 0;
    if (GetAdaptersInfo(NULL, &bufLen) != ERROR_BUFFER_OVERFLOW) {
        return NULL;
    }
    PIP_ADAPTER_INFO list = (PIP_ADAPTER_INFO)malloc(bufLen);
    if (!list) {
        return NULL;
    }
    if (GetAdaptersInfo(list, &bufLen) != NO_ERROR) {
        free(list);
        return NULL;
    }
    return list;
}

static PIP_ADAPTER_INFO FindAdapterByIndex(PIP_ADAPTER_INFO list, DWORD index) {
    for (PIP_ADAPTER_INFO a = list; a; a = a->Next) {
        if (a->Index == index) {
            return a;
        }
    }
    return NULL;
}

static void PopulateInterfaces(HWND hDlg) {
    HWND combo = GetDlgItem(hDlg, IDC_INFO_IFACE_COMBO);
    ComboBox_ResetContent(combo);

    PIP_ADAPTER_INFO list = FetchAdapters();
    for (PIP_ADAPTER_INFO a = list; a; a = a->Next) {
        wchar_t name[256];
        AnsiToWide(a->Description, name, 256);
        int idx = ComboBox_AddString(combo, name);
        ComboBox_SetItemData(combo, idx, a->Index);
    }
    free(list);

    if (ComboBox_GetCount(combo) > 0) {
        ComboBox_SetCurSel(combo, 0);
    }
}

static void RefreshInfo(HWND hDlg) {
    HWND combo = GetDlgItem(hDlg, IDC_INFO_IFACE_COMBO);
    int sel = ComboBox_GetCurSel(combo);
    if (sel == CB_ERR) {
        return;
    }
    DWORD ifIndex = (DWORD)ComboBox_GetItemData(combo, sel);

    PIP_ADAPTER_INFO list = FetchAdapters();
    PIP_ADAPTER_INFO adapter = FindAdapterByIndex(list, ifIndex);

    wchar_t buf[512];

    if (adapter) {
        if (adapter->AddressLength == 6) {
            swprintf(buf, 512, L"%02X-%02X-%02X-%02X-%02X-%02X", adapter->Address[0], adapter->Address[1],
                     adapter->Address[2], adapter->Address[3], adapter->Address[4], adapter->Address[5]);
        } else {
            wcscpy(buf, L"--");
        }
        SetDlgItemTextW(hDlg, IDC_INFO_MAC, buf);

        buf[0] = L'\0';
        for (PIP_ADDR_STRING addr = &adapter->IpAddressList; addr; addr = addr->Next) {
            if (addr->IpAddress.String[0] == '\0' || strcmp(addr->IpAddress.String, "0.0.0.0") == 0) {
                continue;
            }
            wchar_t one[64];
            AnsiToWide(addr->IpAddress.String, one, 64);
            if (buf[0] != L'\0') {
                wcscat(buf, L", ");
            }
            wcscat(buf, one);
        }
        SetDlgItemTextW(hDlg, IDC_INFO_IP, buf[0] ? buf : T(STR_INFO_NONE));
    } else {
        SetDlgItemTextW(hDlg, IDC_INFO_MAC, L"--");
        SetDlgItemTextW(hDlg, IDC_INFO_IP, L"--");
    }
    free(list);

    MIB_IFROW row;
    ZeroMemory(&row, sizeof(row));
    row.dwIndex = ifIndex;
    if (GetIfEntry(&row) == NO_ERROR) {
        if (row.dwSpeed == 0) {
            wcscpy(buf, T(STR_INFO_NA));
        } else {
            swprintf(buf, 512, L"%lu Mb/s", row.dwSpeed / 1000000UL);
        }
        SetDlgItemTextW(hDlg, IDC_INFO_SPEED, buf);

        const wchar_t *statusText = T(STR_INFO_UNKNOWN);
        switch (row.dwOperStatus) {
            case MIB_IF_OPER_STATUS_OPERATIONAL: statusText = T(STR_INFO_ACTIVE_OPERATIONAL); break;
            case MIB_IF_OPER_STATUS_CONNECTED: statusText = T(STR_INFO_ACTIVE_CONNECTED); break;
            case MIB_IF_OPER_STATUS_CONNECTING: statusText = T(STR_INFO_CONNECTING); break;
            case MIB_IF_OPER_STATUS_DISCONNECTED: statusText = T(STR_INFO_INACTIVE_DISCONNECTED); break;
            case MIB_IF_OPER_STATUS_UNREACHABLE: statusText = T(STR_INFO_INACTIVE_UNREACHABLE); break;
            case MIB_IF_OPER_STATUS_NON_OPERATIONAL: statusText = T(STR_INFO_INACTIVE_NONOPERATIONAL); break;
            default: break;
        }
        SetDlgItemTextW(hDlg, IDC_INFO_STATUS, statusText);

        swprintf(buf, 512, L"%lu", row.dwMtu);
        SetDlgItemTextW(hDlg, IDC_INFO_MTU, buf);

        swprintf(buf, 512, L"%lu", row.dwInUcastPkts + row.dwInNUcastPkts);
        SetDlgItemTextW(hDlg, IDC_INFO_RXPACKETS, buf);
        swprintf(buf, 512, L"%lu", row.dwInErrors);
        SetDlgItemTextW(hDlg, IDC_INFO_RXERR, buf);
        swprintf(buf, 512, L"%lu", row.dwOutUcastPkts + row.dwOutNUcastPkts);
        SetDlgItemTextW(hDlg, IDC_INFO_TXPACKETS, buf);
        swprintf(buf, 512, L"%lu", row.dwOutErrors);
        SetDlgItemTextW(hDlg, IDC_INFO_TXERR, buf);
    }
}

INT_PTR CALLBACK InfoTab_DialogProc(HWND hDlg, UINT msg, WPARAM wParam, LPARAM lParam) {
    InfoState *state = (InfoState *)GetWindowLongPtrW(hDlg, DWLP_USER);

    switch (msg) {
        case WM_INITDIALOG:
            state = (InfoState *)calloc(1, sizeof(InfoState));
            SetWindowLongPtrW(hDlg, DWLP_USER, (LONG_PTR)state);
            SetDlgItemTextW(hDlg, IDC_INFO_LABEL_SELECT, T(STR_INFO_SELECT));
            SetDlgItemTextW(hDlg, IDC_INFO_GROUP_IFACE, T(STR_INFO_GROUP_IFACE));
            SetDlgItemTextW(hDlg, IDC_INFO_LABEL_MAC, T(STR_INFO_MAC));
            SetDlgItemTextW(hDlg, IDC_INFO_LABEL_IP, T(STR_INFO_IP));
            SetDlgItemTextW(hDlg, IDC_INFO_LABEL_SPEED, T(STR_INFO_LINKSPEED));
            SetDlgItemTextW(hDlg, IDC_INFO_LABEL_STATUS, T(STR_INFO_LINKSTATUS));
            SetDlgItemTextW(hDlg, IDC_INFO_LABEL_MTU, T(STR_INFO_MTU));
            SetDlgItemTextW(hDlg, IDC_INFO_GROUP_STATS, T(STR_INFO_GROUP_STATS));
            SetDlgItemTextW(hDlg, IDC_INFO_LABEL_RXPACKETS, T(STR_INFO_RXPACKETS));
            SetDlgItemTextW(hDlg, IDC_INFO_LABEL_RXERR, T(STR_INFO_RXERR));
            SetDlgItemTextW(hDlg, IDC_INFO_LABEL_TXPACKETS, T(STR_INFO_TXPACKETS));
            SetDlgItemTextW(hDlg, IDC_INFO_LABEL_TXERR, T(STR_INFO_TXERR));

            PopulateInterfaces(hDlg);
            RefreshInfo(hDlg);
            SetTimer(hDlg, REFRESH_TIMER_ID, 2000, NULL);
            state->resizer = DialogResize_Init(hDlg, kAnchors, (int)(sizeof(kAnchors) / sizeof(kAnchors[0])));
            return TRUE;

        case WM_TIMER:
            if (wParam == REFRESH_TIMER_ID) {
                RefreshInfo(hDlg);
            }
            return TRUE;

        case WM_SIZE:
            if (state) {
                DialogResize_Apply(state->resizer, LOWORD(lParam), HIWORD(lParam));
            }
            return TRUE;

        case WM_COMMAND:
            if (LOWORD(wParam) == IDC_INFO_IFACE_COMBO && HIWORD(wParam) == CBN_SELCHANGE) {
                RefreshInfo(hDlg);
            }
            return TRUE;

        case WM_DESTROY:
            KillTimer(hDlg, REFRESH_TIMER_ID);
            if (state) {
                DialogResize_Free(state->resizer);
                free(state);
                SetWindowLongPtrW(hDlg, DWLP_USER, 0);
            }
            return TRUE;

        default:
            break;
    }
    (void)lParam;
    return FALSE;
}
