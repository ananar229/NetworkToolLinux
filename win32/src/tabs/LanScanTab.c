#include "LanScanTab.h"

#include <winsock2.h>
#include <ws2tcpip.h>

#include <commctrl.h>
#include <ipexport.h>
#include <icmpapi.h>
#include <iphlpapi.h>
#include <netioapi.h>
#include <stdio.h>
#include <stdlib.h>
#include <wchar.h>
#include <windowsx.h>

#include "common/OuiVendors.h"
#include "common/Translations.h"
#include "common/resource.h"

#define SCAN_THREAD_COUNT 32
#define MAX_SCAN_HOSTS 1024
#define MAX_IPV6_GROUPS 256

typedef struct {
    HWND hDlg;
    DWORD networkAddr; /* network base address, host byte order */
    DWORD hostCount;
    volatile LONG nextOffset;
    volatile LONG completedCount;
    volatile LONG *stopFlag;
} ScanContext;

typedef struct {
    HANDLE hCoordinatorThread;
    volatile LONG stopRequested;
} LanScanState;

typedef struct {
    wchar_t ip[16];
    wchar_t hostname[256];
    wchar_t mac[18];
    wchar_t vendor[64];
    BOOL pingOk;
} LanScanHostResult;

typedef struct {
    wchar_t mac[18];
    wchar_t local[256];
    wchar_t global[256];
} LanScanIpv6Result;

typedef NETIO_STATUS(WINAPI *GetIpNetTable2Func)(ADDRESS_FAMILY, PMIB_IPNET_TABLE2 *);
typedef VOID(WINAPI *FreeMibTableFunc)(PVOID);

static void FormatMac(wchar_t *out, size_t outSize, const BYTE mac[6]) {
    swprintf(out, outSize, L"%02X:%02X:%02X:%02X:%02X:%02X", mac[0], mac[1], mac[2], mac[3], mac[4], mac[5]);
}

/* Best-effort: cross-references the system's IPv6 neighbor cache (populated
 * by whatever IPv6 traffic already happened, since there is no lightweight
 * unprivileged way to actively trigger neighbor discovery here) against the
 * MAC addresses found via ARP, and posts one update per matched MAC. Silently
 * does nothing if GetIpNetTable2 isn't available (pre-Vista Windows, and
 * likely ReactOS, whose iphlpapi.dll doesn't export it). */
static void PostIpv6Neighbors(HWND hDlg, volatile LONG *stopFlag) {
    HMODULE hIphlpapi = GetModuleHandleW(L"iphlpapi.dll");
    if (!hIphlpapi) {
        return;
    }
    GetIpNetTable2Func pGetIpNetTable2 = (GetIpNetTable2Func)GetProcAddress(hIphlpapi, "GetIpNetTable2");
    FreeMibTableFunc pFreeMibTable = (FreeMibTableFunc)GetProcAddress(hIphlpapi, "FreeMibTable");
    if (!pGetIpNetTable2 || !pFreeMibTable) {
        return;
    }

    PMIB_IPNET_TABLE2 table = NULL;
    if (pGetIpNetTable2(AF_INET6, &table) != NO_ERROR || !table) {
        return;
    }

    LanScanIpv6Result *groups = (LanScanIpv6Result *)calloc(MAX_IPV6_GROUPS, sizeof(LanScanIpv6Result));
    int groupCount = 0;

    for (ULONG i = 0; i < table->NumEntries && !*stopFlag; i++) {
        const MIB_IPNET_ROW2 *row = &table->Table[i];
        if (row->PhysicalAddressLength != 6 || row->Address.si_family != AF_INET6) {
            continue;
        }
        const BYTE *b = row->Address.Ipv6.sin6_addr.u.Byte;
        const BOOL isLinkLocal = (b[0] == 0xFE && (b[1] & 0xC0) == 0x80);
        const BOOL isMulticast = (b[0] == 0xFF);
        if (isMulticast) {
            continue;
        }

        wchar_t addrText[64];
        DWORD addrTextLen = 64;
        WSAAddressToStringW((LPSOCKADDR)&row->Address, sizeof(SOCKADDR_IN6), NULL, addrText, &addrTextLen);

        wchar_t mac[18];
        FormatMac(mac, 18, row->PhysicalAddress);

        int groupIndex = -1;
        for (int g = 0; g < groupCount; g++) {
            if (_wcsicmp(groups[g].mac, mac) == 0) {
                groupIndex = g;
                break;
            }
        }
        if (groupIndex < 0 && groupCount < MAX_IPV6_GROUPS) {
            groupIndex = groupCount++;
            wcscpy(groups[groupIndex].mac, mac);
        }
        if (groupIndex < 0) {
            continue;
        }

        wchar_t *target = isLinkLocal ? groups[groupIndex].local : groups[groupIndex].global;
        const size_t capacity = 256;
        if (target[0] != L'\0' && wcslen(target) + 2 < capacity) {
            wcscat(target, L", ");
        }
        if (wcslen(target) + wcslen(addrText) < capacity) {
            wcscat(target, addrText);
        }
    }

    for (int g = 0; g < groupCount && !*stopFlag; g++) {
        LanScanIpv6Result *copy = (LanScanIpv6Result *)malloc(sizeof(LanScanIpv6Result));
        *copy = groups[g];
        PostMessageW(hDlg, WM_APP_LANSCAN_IPV6, 0, (LPARAM)copy);
    }

    free(groups);
    pFreeMibTable(table);
}

static DWORD WINAPI ScanWorker(LPVOID param) {
    ScanContext *ctx = (ScanContext *)param;
    HANDLE hIcmp = IcmpCreateFile();
    char sendData[8] = "NTLan";
    const DWORD replySize = sizeof(ICMP_ECHO_REPLY) + sizeof(sendData) + 32;
    LPVOID replyBuffer = malloc(replySize);

    for (;;) {
        if (*ctx->stopFlag) {
            break;
        }
        LONG offset = InterlockedIncrement(&ctx->nextOffset) - 1;
        if ((DWORD)offset > ctx->hostCount || offset < 1) {
            break;
        }

        const DWORD hostAddrHost = ctx->networkAddr + (DWORD)offset;
        const DWORD destAddr = htonl(hostAddrHost);

        BOOL pingOk = FALSE;
        if (hIcmp != INVALID_HANDLE_VALUE) {
            DWORD ret = IcmpSendEcho(hIcmp, destAddr, sendData, (WORD)sizeof(sendData), NULL, replyBuffer, replySize, 500);
            pingOk = (ret > 0);
        }

        BYTE mac[6] = {0};
        ULONG macLen = 6;
        if (SendARP(destAddr, 0, mac, &macLen) == NO_ERROR && macLen == 6 &&
            !(mac[0] == 0 && mac[1] == 0 && mac[2] == 0 && mac[3] == 0 && mac[4] == 0 && mac[5] == 0)) {
            LanScanHostResult *result = (LanScanHostResult *)calloc(1, sizeof(LanScanHostResult));
            struct in_addr a;
            a.S_un.S_addr = destAddr;
            MultiByteToWideChar(CP_ACP, 0, inet_ntoa(a), -1, result->ip, 16);
            FormatMac(result->mac, 18, mac);
            result->pingOk = pingOk;

            const wchar_t *vendor = OuiVendors_Lookup(mac);
            if (vendor) {
                wcsncpy(result->vendor, vendor, 63);
            }

            struct sockaddr_in sa;
            ZeroMemory(&sa, sizeof(sa));
            sa.sin_family = AF_INET;
            sa.sin_addr.S_un.S_addr = destAddr;
            wchar_t hostBuf[NI_MAXHOST];
            if (GetNameInfoW((SOCKADDR *)&sa, sizeof(sa), hostBuf, NI_MAXHOST, NULL, 0, NI_NAMEREQD) == 0) {
                wcsncpy(result->hostname, hostBuf, 255);
            }

            PostMessageW(ctx->hDlg, WM_APP_LANSCAN_HOST, 0, (LPARAM)result);
        }

        LONG completed = InterlockedIncrement(&ctx->completedCount);
        PostMessageW(ctx->hDlg, WM_APP_LANSCAN_PROGRESS, (WPARAM)completed, (LPARAM)ctx->hostCount);
    }

    free(replyBuffer);
    if (hIcmp != INVALID_HANDLE_VALUE) {
        IcmpCloseHandle(hIcmp);
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

    if (!*ctx->stopFlag) {
        PostIpv6Neighbors(ctx->hDlg, ctx->stopFlag);
    }

    PostMessageW(ctx->hDlg, WM_APP_LANSCAN_DONE, 0, 0);
    free(ctx);
    return 0;
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

static void PopulateInterfaces(HWND hDlg) {
    HWND combo = GetDlgItem(hDlg, IDC_LANSCAN_IFACE_COMBO);
    ComboBox_ResetContent(combo);

    PIP_ADAPTER_INFO list = FetchAdapters();
    for (PIP_ADAPTER_INFO a = list; a; a = a->Next) {
        if (a->IpAddressList.IpAddress.String[0] == '\0' ||
            strcmp(a->IpAddressList.IpAddress.String, "0.0.0.0") == 0) {
            continue;
        }
        wchar_t name[256];
        MultiByteToWideChar(CP_ACP, 0, a->Description, -1, name, 256);
        int idx = ComboBox_AddString(combo, name);
        ComboBox_SetItemData(combo, idx, a->Index);
    }
    free(list);

    if (ComboBox_GetCount(combo) > 0) {
        ComboBox_SetCurSel(combo, 0);
    }
}

static void SetupListView(HWND hDlg) {
    HWND hList = GetDlgItem(hDlg, IDC_LANSCAN_LIST);
    ListView_SetExtendedListViewStyle(hList, LVS_EX_FULLROWSELECT | LVS_EX_GRIDLINES);

    LVCOLUMNW col;
    ZeroMemory(&col, sizeof(col));
    col.mask = LVCF_TEXT | LVCF_WIDTH | LVCF_SUBITEM;

    struct {
        int width;
        StringId strId;
    } columns[] = {
        {90, STR_LANSCAN_COL_IPV4},   {110, STR_LANSCAN_COL_HOSTNAME},     {70, STR_LANSCAN_COL_PING},
        {110, STR_LANSCAN_COL_IPV6LOCAL}, {110, STR_LANSCAN_COL_IPV6GLOBAL}, {110, STR_LANSCAN_COL_MAC},
        {100, STR_LANSCAN_COL_VENDOR},
    };
    for (int i = 0; i < (int)(sizeof(columns) / sizeof(columns[0])); i++) {
        col.iSubItem = i;
        col.cx = columns[i].width;
        col.pszText = (wchar_t *)T(columns[i].strId);
        ListView_InsertColumn(hList, i, &col);
    }
}

static void StartScan(HWND hDlg, LanScanState *state) {
    HWND combo = GetDlgItem(hDlg, IDC_LANSCAN_IFACE_COMBO);
    int sel = ComboBox_GetCurSel(combo);
    if (sel == CB_ERR) {
        return;
    }
    DWORD ifIndex = (DWORD)ComboBox_GetItemData(combo, sel);

    PIP_ADAPTER_INFO list = FetchAdapters();
    PIP_ADAPTER_INFO adapter = NULL;
    for (PIP_ADAPTER_INFO a = list; a; a = a->Next) {
        if (a->Index == ifIndex) {
            adapter = a;
            break;
        }
    }
    if (!adapter) {
        free(list);
        return;
    }

    const DWORD ipHost = ntohl(inet_addr(adapter->IpAddressList.IpAddress.String));
    const DWORD maskHost = ntohl(inet_addr(adapter->IpAddressList.IpMask.String));
    free(list);

    const DWORD network = ipHost & maskHost;
    const DWORD broadcast = network | ~maskHost;
    DWORD hostCount = (broadcast > network + 1) ? (broadcast - network - 1) : 0;
    if (hostCount > MAX_SCAN_HOSTS) {
        hostCount = MAX_SCAN_HOSTS;
    }

    HWND hList = GetDlgItem(hDlg, IDC_LANSCAN_LIST);
    ListView_DeleteAllItems(hList);

    HWND progress = GetDlgItem(hDlg, IDC_LANSCAN_PROGRESS);
    SendMessageW(progress, PBM_SETRANGE32, 0, hostCount > 0 ? hostCount : 1);
    SendMessageW(progress, PBM_SETPOS, 0, 0);

    state->stopRequested = 0;

    ScanContext *ctx = (ScanContext *)calloc(1, sizeof(ScanContext));
    ctx->hDlg = hDlg;
    ctx->networkAddr = network;
    ctx->hostCount = hostCount;
    ctx->nextOffset = 1;
    ctx->stopFlag = &state->stopRequested;

    SetDlgItemTextW(hDlg, IDC_LANSCAN_BTN, T(STR_LANSCAN_STOP));
    EnableWindow(combo, FALSE);
    state->hCoordinatorThread = CreateThread(NULL, 0, ScanCoordinator, ctx, 0, NULL);
}

static int FindRowByMac(HWND hList, const wchar_t *mac) {
    const int rowCount = ListView_GetItemCount(hList);
    wchar_t buf[18];
    for (int row = 0; row < rowCount; row++) {
        ListView_GetItemText(hList, row, 5, buf, 18);
        if (_wcsicmp(buf, mac) == 0) {
            return row;
        }
    }
    return -1;
}

INT_PTR CALLBACK LanScanTab_DialogProc(HWND hDlg, UINT msg, WPARAM wParam, LPARAM lParam) {
    LanScanState *state = (LanScanState *)GetWindowLongPtrW(hDlg, DWLP_USER);

    switch (msg) {
        case WM_INITDIALOG:
            state = (LanScanState *)calloc(1, sizeof(LanScanState));
            SetWindowLongPtrW(hDlg, DWLP_USER, (LONG_PTR)state);
            SetDlgItemTextW(hDlg, IDC_LANSCAN_LABEL_IFACE, T(STR_LANSCAN_LABEL_IFACE));
            SetDlgItemTextW(hDlg, IDC_LANSCAN_BTN, T(STR_LANSCAN_BTN));
            SetupListView(hDlg);
            PopulateInterfaces(hDlg);
            return TRUE;

        case WM_COMMAND:
            if (LOWORD(wParam) == IDC_LANSCAN_BTN) {
                if (state->hCoordinatorThread) {
                    InterlockedExchange(&state->stopRequested, 1);
                } else {
                    StartScan(hDlg, state);
                }
            }
            return TRUE;

        case WM_APP_LANSCAN_HOST: {
            LanScanHostResult *r = (LanScanHostResult *)lParam;
            HWND hList = GetDlgItem(hDlg, IDC_LANSCAN_LIST);

            LVITEMW item;
            ZeroMemory(&item, sizeof(item));
            item.mask = LVIF_TEXT | LVIF_PARAM;
            item.iItem = ListView_GetItemCount(hList);
            item.iSubItem = 0;
            item.pszText = r->ip;
            item.lParam = r->pingOk ? 1 : 0;
            int row = ListView_InsertItem(hList, &item);

            ListView_SetItemText(hList, row, 1, r->hostname[0] ? r->hostname : (wchar_t *)T(STR_LANSCAN_NONE));
            ListView_SetItemText(hList, row, 2,
                                  (wchar_t *)(r->pingOk ? T(STR_LANSCAN_REACHABLE) : T(STR_LANSCAN_NOREPLY)));
            ListView_SetItemText(hList, row, 3, (wchar_t *)T(STR_LANSCAN_NONE));
            ListView_SetItemText(hList, row, 4, (wchar_t *)T(STR_LANSCAN_NONE));
            ListView_SetItemText(hList, row, 5, r->mac);
            ListView_SetItemText(hList, row, 6, r->vendor[0] ? r->vendor : (wchar_t *)T(STR_LANSCAN_UNKNOWN));

            free(r);
            return TRUE;
        }

        case WM_APP_LANSCAN_IPV6: {
            LanScanIpv6Result *r = (LanScanIpv6Result *)lParam;
            HWND hList = GetDlgItem(hDlg, IDC_LANSCAN_LIST);
            int row = FindRowByMac(hList, r->mac);
            if (row >= 0) {
                if (r->local[0]) {
                    ListView_SetItemText(hList, row, 3, r->local);
                }
                if (r->global[0]) {
                    ListView_SetItemText(hList, row, 4, r->global);
                }
            }
            free(r);
            return TRUE;
        }

        case WM_APP_LANSCAN_PROGRESS:
            SendMessageW(GetDlgItem(hDlg, IDC_LANSCAN_PROGRESS), PBM_SETPOS, wParam, 0);
            return TRUE;

        case WM_APP_LANSCAN_DONE:
            if (state->hCoordinatorThread) {
                CloseHandle(state->hCoordinatorThread);
                state->hCoordinatorThread = NULL;
            }
            SetDlgItemTextW(hDlg, IDC_LANSCAN_BTN, T(STR_LANSCAN_BTN));
            EnableWindow(GetDlgItem(hDlg, IDC_LANSCAN_IFACE_COMBO), TRUE);
            return TRUE;

        case WM_NOTIFY: {
            LPNMHDR nmhdr = (LPNMHDR)lParam;
            if (nmhdr->idFrom != (UINT_PTR)IDC_LANSCAN_LIST || nmhdr->code != NM_CUSTOMDRAW) {
                return FALSE;
            }
            LPNMLVCUSTOMDRAW cd = (LPNMLVCUSTOMDRAW)lParam;
            switch (cd->nmcd.dwDrawStage) {
                case CDDS_PREPAINT:
                    SetWindowLongPtrW(hDlg, DWLP_MSGRESULT, CDRF_NOTIFYITEMDRAW);
                    return TRUE;
                case CDDS_ITEMPREPAINT:
                    SetWindowLongPtrW(hDlg, DWLP_MSGRESULT, CDRF_NOTIFYSUBITEMDRAW);
                    return TRUE;
                case CDDS_ITEMPREPAINT | CDDS_SUBITEM:
                    if (cd->iSubItem == 2) {
                        cd->clrText = cd->nmcd.lItemlParam ? RGB(0, 140, 0) : RGB(190, 0, 0);
                    }
                    SetWindowLongPtrW(hDlg, DWLP_MSGRESULT, CDRF_NEWFONT);
                    return TRUE;
                default:
                    SetWindowLongPtrW(hDlg, DWLP_MSGRESULT, CDRF_DODEFAULT);
                    return TRUE;
            }
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

        default:
            break;
    }
    return FALSE;
}
