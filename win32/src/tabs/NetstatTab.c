#include "NetstatTab.h"

#include <iphlpapi.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <winsock2.h>

#include "common/DialogResize.h"
#include "common/OutputEdit.h"
#include "common/Translations.h"
#include "common/resource.h"

typedef struct {
    DialogResizer *resizer;
} NetstatState;

static const ResizeAnchor kAnchors[] = {
    {IDC_NET_RUN, RESIZE_RIGHT | RESIZE_TOP},
    {IDC_NET_OUTPUT, RESIZE_LEFT | RESIZE_TOP | RESIZE_RIGHT | RESIZE_BOTTOM},
};

static const wchar_t *TcpStateName(DWORD state) {
    switch (state) {
        case MIB_TCP_STATE_CLOSED: return L"CLOSED";
        case MIB_TCP_STATE_LISTEN: return L"LISTEN";
        case MIB_TCP_STATE_SYN_SENT: return L"SYN_SENT";
        case MIB_TCP_STATE_SYN_RCVD: return L"SYN_RCVD";
        case MIB_TCP_STATE_ESTAB: return L"ESTABLISHED";
        case MIB_TCP_STATE_FIN_WAIT1: return L"FIN_WAIT1";
        case MIB_TCP_STATE_FIN_WAIT2: return L"FIN_WAIT2";
        case MIB_TCP_STATE_CLOSE_WAIT: return L"CLOSE_WAIT";
        case MIB_TCP_STATE_CLOSING: return L"CLOSING";
        case MIB_TCP_STATE_LAST_ACK: return L"LAST_ACK";
        case MIB_TCP_STATE_TIME_WAIT: return L"TIME_WAIT";
        case MIB_TCP_STATE_DELETE_TCB: return L"DELETE_TCB";
        default: return L"UNKNOWN";
    }
}

static void FormatAddrPort(DWORD addr, USHORT port, wchar_t *out, int outCount) {
    struct in_addr a;
    a.S_un.S_addr = addr;
    char ansi[32];
    strcpy(ansi, inet_ntoa(a));
    wchar_t wide[32];
    MultiByteToWideChar(CP_ACP, 0, ansi, -1, wide, 32);
    swprintf(out, outCount, L"%s:%u", wide, ntohs(port));
}

static void RunRoutingTable(HWND output) {
    OutputEdit_AppendLine(output, T(STR_NET_ROUTE_HEADER));

    ULONG size = 0;
    if (GetIpForwardTable(NULL, &size, FALSE) != ERROR_INSUFFICIENT_BUFFER) {
        OutputEdit_AppendLine(output, T(STR_NET_NOROUTE));
        return;
    }
    PMIB_IPFORWARDTABLE table = (PMIB_IPFORWARDTABLE)malloc(size);
    if (!table || GetIpForwardTable(table, &size, FALSE) != NO_ERROR) {
        free(table);
        return;
    }

    for (DWORD i = 0; i < table->dwNumEntries; i++) {
        MIB_IPFORWARDROW *row = &table->table[i];
        wchar_t dest[32], mask[32], gw[32];
        struct in_addr a;
        a.S_un.S_addr = row->dwForwardDest;
        MultiByteToWideChar(CP_ACP, 0, inet_ntoa(a), -1, dest, 32);
        a.S_un.S_addr = row->dwForwardMask;
        MultiByteToWideChar(CP_ACP, 0, inet_ntoa(a), -1, mask, 32);
        a.S_un.S_addr = row->dwForwardNextHop;
        MultiByteToWideChar(CP_ACP, 0, inet_ntoa(a), -1, gw, 32);

        wchar_t line[160];
        swprintf(line, 160, L"%-18s %-18s %-18s %-10lu %lu", dest, mask, gw, row->dwForwardIfIndex,
                 row->dwForwardMetric1);
        OutputEdit_AppendLine(output, line);
    }
    free(table);
}

static void RunProtocolStatistics(HWND output) {
    MIB_IPSTATS ipStats;
    MIB_TCPSTATS tcpStats;
    MIB_UDPSTATS udpStats;
    wchar_t line[160];

    if (GetIpStatistics(&ipStats) == NO_ERROR) {
        OutputEdit_AppendLine(output, T(STR_NET_IPSTATS));
        swprintf(line, 160, L"%s %lu", T(STR_NET_PKT_RECEIVED), ipStats.dwInReceives);
        OutputEdit_AppendLine(output, line);
        swprintf(line, 160, L"%s %lu", T(STR_NET_PKT_FORWARDED), ipStats.dwForwDatagrams);
        OutputEdit_AppendLine(output, line);
        swprintf(line, 160, L"%s %lu", T(STR_NET_PKT_DELIVERED), ipStats.dwInDelivers);
        OutputEdit_AppendLine(output, line);
        swprintf(line, 160, L"%s %lu", T(STR_NET_PKT_SENT), ipStats.dwOutRequests);
        OutputEdit_AppendLine(output, line);
        swprintf(line, 160, L"%s %lu / %lu", T(STR_NET_DISCARDED), ipStats.dwInDiscards, ipStats.dwOutDiscards);
        OutputEdit_AppendLine(output, line);
        OutputEdit_AppendLine(output, L"");
    }

    if (GetTcpStatistics(&tcpStats) == NO_ERROR) {
        OutputEdit_AppendLine(output, T(STR_NET_TCPSTATS));
        swprintf(line, 160, L"%s %lu", T(STR_NET_ACTIVE_OPENS), tcpStats.dwActiveOpens);
        OutputEdit_AppendLine(output, line);
        swprintf(line, 160, L"%s %lu", T(STR_NET_PASSIVE_OPENS), tcpStats.dwPassiveOpens);
        OutputEdit_AppendLine(output, line);
        swprintf(line, 160, L"%s %lu", T(STR_NET_FAILED_CONN), tcpStats.dwAttemptFails);
        OutputEdit_AppendLine(output, line);
        swprintf(line, 160, L"%s %lu", T(STR_NET_RESETS_SENT), tcpStats.dwOutRsts);
        OutputEdit_AppendLine(output, line);
        swprintf(line, 160, L"%s %lu", T(STR_NET_SEGMENTS_RECEIVED), tcpStats.dwInSegs);
        OutputEdit_AppendLine(output, line);
        swprintf(line, 160, L"%s %lu", T(STR_NET_SEGMENTS_SENT), tcpStats.dwOutSegs);
        OutputEdit_AppendLine(output, line);
        swprintf(line, 160, L"%s %lu", T(STR_NET_SEGMENTS_RETRANS), tcpStats.dwRetransSegs);
        OutputEdit_AppendLine(output, line);
        swprintf(line, 160, L"%s %lu", T(STR_NET_CURRENT_CONN), tcpStats.dwCurrEstab);
        OutputEdit_AppendLine(output, line);
        OutputEdit_AppendLine(output, L"");
    }

    if (GetUdpStatistics(&udpStats) == NO_ERROR) {
        OutputEdit_AppendLine(output, T(STR_NET_UDPSTATS));
        swprintf(line, 160, L"%s %lu", T(STR_NET_DGRAMS_RECEIVED), udpStats.dwInDatagrams);
        OutputEdit_AppendLine(output, line);
        swprintf(line, 160, L"%s %lu", T(STR_NET_NOPORT), udpStats.dwNoPorts);
        OutputEdit_AppendLine(output, line);
        swprintf(line, 160, L"%s %lu", T(STR_NET_DGRAMS_SENT), udpStats.dwOutDatagrams);
        OutputEdit_AppendLine(output, line);
    }
}

static void RunActiveConnections(HWND output) {
    OutputEdit_AppendLine(output, T(STR_NET_CONN_HEADER));

    ULONG size = 0;
    if (GetTcpTable(NULL, &size, TRUE) == ERROR_INSUFFICIENT_BUFFER) {
        PMIB_TCPTABLE table = (PMIB_TCPTABLE)malloc(size);
        if (table && GetTcpTable(table, &size, TRUE) == NO_ERROR) {
            for (DWORD i = 0; i < table->dwNumEntries; i++) {
                MIB_TCPROW *row = &table->table[i];
                wchar_t local[32], remote[32], line[160];
                FormatAddrPort(row->dwLocalAddr, (USHORT)row->dwLocalPort, local, 32);
                FormatAddrPort(row->dwRemoteAddr, (USHORT)row->dwRemotePort, remote, 32);
                swprintf(line, 160, L"TCP    %-23s %-23s %s", local, remote, TcpStateName(row->dwState));
                OutputEdit_AppendLine(output, line);
            }
        }
        free(table);
    }

    size = 0;
    if (GetUdpTable(NULL, &size, TRUE) == ERROR_INSUFFICIENT_BUFFER) {
        PMIB_UDPTABLE table = (PMIB_UDPTABLE)malloc(size);
        if (table && GetUdpTable(table, &size, TRUE) == NO_ERROR) {
            for (DWORD i = 0; i < table->dwNumEntries; i++) {
                MIB_UDPROW *row = &table->table[i];
                wchar_t local[32], line[160];
                FormatAddrPort(row->dwLocalAddr, (USHORT)row->dwLocalPort, local, 32);
                swprintf(line, 160, L"UDP    %-23s %-23s %s", local, L"*:*", L"");
                OutputEdit_AppendLine(output, line);
            }
        }
        free(table);
    }
}

INT_PTR CALLBACK NetstatTab_DialogProc(HWND hDlg, UINT msg, WPARAM wParam, LPARAM lParam) {
    NetstatState *state = (NetstatState *)GetWindowLongPtrW(hDlg, DWLP_USER);

    switch (msg) {
        case WM_INITDIALOG:
            state = (NetstatState *)calloc(1, sizeof(NetstatState));
            SetWindowLongPtrW(hDlg, DWLP_USER, (LONG_PTR)state);
            SetDlgItemTextW(hDlg, IDC_NET_RADIO_ROUTE, T(STR_NET_ROUTE));
            SetDlgItemTextW(hDlg, IDC_NET_RADIO_STATS, T(STR_NET_STATS));
            SetDlgItemTextW(hDlg, IDC_NET_RADIO_CONN, T(STR_NET_CONN));
            SetDlgItemTextW(hDlg, IDC_NET_RUN, T(STR_NET_BTN));
            CheckRadioButton(hDlg, IDC_NET_RADIO_ROUTE, IDC_NET_RADIO_CONN, IDC_NET_RADIO_ROUTE);
            state->resizer = DialogResize_Init(hDlg, kAnchors, (int)(sizeof(kAnchors) / sizeof(kAnchors[0])));
            return TRUE;

        case WM_SIZE:
            if (state) {
                DialogResize_Apply(state->resizer, LOWORD(lParam), HIWORD(lParam));
            }
            return TRUE;

        case WM_COMMAND:
            if (LOWORD(wParam) == IDC_NET_RUN) {
                HWND output = GetDlgItem(hDlg, IDC_NET_OUTPUT);
                OutputEdit_Clear(output);
                if (IsDlgButtonChecked(hDlg, IDC_NET_RADIO_ROUTE)) {
                    RunRoutingTable(output);
                } else if (IsDlgButtonChecked(hDlg, IDC_NET_RADIO_STATS)) {
                    RunProtocolStatistics(output);
                } else {
                    RunActiveConnections(output);
                }
            }
            return TRUE;

        case WM_DESTROY:
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
