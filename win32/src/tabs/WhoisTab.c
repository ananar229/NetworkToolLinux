#include "WhoisTab.h"

#include <stdio.h>
#include <wchar.h>
#include <windowsx.h>

#include "common/OutputEdit.h"
#include "common/TcpQuery.h"
#include "common/Translations.h"
#include "common/resource.h"

typedef struct {
    const wchar_t *server; /* NULL = automatic (referred via whois.iana.org) */
} WhoisServerSpec;

static const WhoisServerSpec kServers[] = {
    {NULL}, {L"whois.internic.net"}, {L"whois.arin.net"}, {L"whois.iana.org"}, {L"whois.ripe.net"},
};
#define SERVER_COUNT (int)(sizeof(kServers) / sizeof(kServers[0]))

/* Looks for a "whois:        <server>" referral line in an IANA response. */
static BOOL FindReferral(const wchar_t *text, wchar_t *outServer, int outServerSize) {
    const wchar_t *marker = wcsstr(text, L"whois:");
    if (!marker) {
        return FALSE;
    }
    marker += 6;
    while (*marker == L' ' || *marker == L'\t') {
        marker++;
    }
    int i = 0;
    while (*marker && *marker != L'\r' && *marker != L'\n' && i < outServerSize - 1) {
        outServer[i++] = *marker++;
    }
    outServer[i] = L'\0';
    return i > 0;
}

static void RunWhois(HWND hDlg) {
    wchar_t address[256];
    GetDlgItemTextW(hDlg, IDC_WHOIS_ADDR, address, 256);
    HWND output = GetDlgItem(hDlg, IDC_WHOIS_OUTPUT);
    OutputEdit_Clear(output);
    if (wcslen(address) == 0) {
        OutputEdit_AppendLine(output, T(STR_WHOIS_NOADDR));
        return;
    }

    HWND combo = GetDlgItem(hDlg, IDC_WHOIS_SERVER);
    int sel = ComboBox_GetCurSel(combo);
    const wchar_t *server = sel > 0 ? kServers[sel].server : NULL;

    wchar_t errorMsg[256];
    wchar_t *response = NULL;
    wchar_t line[300];

    if (server) {
        swprintf(line, 300, T(STR_WHOIS_QUERYING), server);
        wcscat(line, L"\r\n");
        OutputEdit_Append(output, line);
        if (TcpQuery_SendReceive(server, 43, address, &response, errorMsg, 256)) {
            OutputEdit_Append(output, response);
            free(response);
        } else {
            OutputEdit_AppendLine(output, errorMsg);
        }
        return;
    }

    swprintf(line, 300, T(STR_WHOIS_QUERYING), L"whois.iana.org");
    wcscat(line, L"\r\n");
    OutputEdit_Append(output, line);
    if (!TcpQuery_SendReceive(L"whois.iana.org", 43, address, &response, errorMsg, 256)) {
        OutputEdit_AppendLine(output, errorMsg);
        return;
    }

    wchar_t referral[128];
    if (FindReferral(response, referral, 128)) {
        free(response);
        response = NULL;
        swprintf(line, 300, T(STR_WHOIS_QUERYING), referral);
        wcscat(line, L"\r\n");
        OutputEdit_Append(output, line);
        if (TcpQuery_SendReceive(referral, 43, address, &response, errorMsg, 256)) {
            OutputEdit_Append(output, response);
            free(response);
        } else {
            OutputEdit_AppendLine(output, errorMsg);
        }
    } else {
        OutputEdit_Append(output, response);
        free(response);
    }
}

INT_PTR CALLBACK WhoisTab_DialogProc(HWND hDlg, UINT msg, WPARAM wParam, LPARAM lParam) {
    switch (msg) {
        case WM_INITDIALOG: {
            SetDlgItemTextW(hDlg, IDC_WHOIS_LABEL_ADDR, T(STR_WHOIS_LABEL));
            SetDlgItemTextW(hDlg, IDC_WHOIS_LABEL_SERVER, T(STR_WHOIS_SERVERLABEL));
            SetDlgItemTextW(hDlg, IDC_WHOIS_BTN, T(STR_WHOIS_BTN));

            HWND combo = GetDlgItem(hDlg, IDC_WHOIS_SERVER);
            ComboBox_AddString(combo, T(STR_WHOIS_AUTOMATIC));
            for (int i = 1; i < SERVER_COUNT; i++) {
                ComboBox_AddString(combo, kServers[i].server);
            }
            ComboBox_SetCurSel(combo, 0);
            return TRUE;
        }

        case WM_COMMAND:
            if (LOWORD(wParam) == IDC_WHOIS_BTN) {
                RunWhois(hDlg);
            }
            return TRUE;

        default:
            break;
    }
    (void)lParam;
    return FALSE;
}
