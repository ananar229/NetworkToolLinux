#include "LookupTab.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <windns.h>
#include <windowsx.h>
#include <winsock2.h>
#include <ws2tcpip.h>

#include "common/DialogResize.h"
#include "common/OutputEdit.h"
#include "common/Translations.h"
#include "common/resource.h"

typedef struct {
    DialogResizer *resizer;
} LookupState;

static const ResizeAnchor kAnchors[] = {
    {IDC_LOOKUP_ADDR, RESIZE_LEFT | RESIZE_TOP | RESIZE_RIGHT},
    {IDC_LOOKUP_BTN, RESIZE_RIGHT | RESIZE_TOP},
    {IDC_LOOKUP_OUTPUT, RESIZE_LEFT | RESIZE_TOP | RESIZE_RIGHT | RESIZE_BOTTOM},
};

typedef struct {
    StringId label;
    WORD dnsType; /* 0 = default A/AAAA via getaddrinfo */
} LookupTypeSpec;

static const LookupTypeSpec kTypes[] = {
    {STR_LOOKUP_TYPE_DEFAULT, 0},
    {STR_LOOKUP_TYPE_A, DNS_TYPE_A},
    {STR_LOOKUP_TYPE_AAAA, DNS_TYPE_AAAA},
    {STR_LOOKUP_TYPE_MX, DNS_TYPE_MX},
    {STR_LOOKUP_TYPE_NS, DNS_TYPE_NS},
    {STR_LOOKUP_TYPE_TXT, DNS_TYPE_TEXT},
    {STR_LOOKUP_TYPE_CNAME, DNS_TYPE_CNAME},
    {STR_LOOKUP_TYPE_SOA, DNS_TYPE_SOA},
};
#define TYPE_COUNT (int)(sizeof(kTypes) / sizeof(kTypes[0]))

static void RunDefaultLookup(HWND output, const wchar_t *address) {
    ADDRINFOW hints;
    ZeroMemory(&hints, sizeof(hints));
    hints.ai_family = AF_UNSPEC;

    ADDRINFOW *result = NULL;
    int ret = GetAddrInfoW(address, NULL, &hints, &result);
    if (ret != 0) {
        OutputEdit_AppendLine(output, T(STR_LOOKUP_FAILED));
        return;
    }

    for (ADDRINFOW *ai = result; ai; ai = ai->ai_next) {
        wchar_t text[128];
        DWORD textLen = 128;
        if (WSAAddressToStringW(ai->ai_addr, (DWORD)ai->ai_addrlen, NULL, text, &textLen) == 0) {
            OutputEdit_AppendLine(output, text);
        }
    }
    FreeAddrInfoW(result);
}

static void RunDnsQuery(HWND output, const wchar_t *address, WORD type) {
    PDNS_RECORDW records = NULL;
    DNS_STATUS status = DnsQuery_W(address, type, DNS_QUERY_STANDARD, NULL, &records, NULL);
    if (status != 0) {
        OutputEdit_AppendLine(output, T(STR_LOOKUP_FAILED));
        return;
    }

    for (PDNS_RECORDW r = records; r; r = r->pNext) {
        if (r->wType != type) {
            continue;
        }
        wchar_t line[512];
        switch (type) {
            case DNS_TYPE_A: {
                struct in_addr a;
                a.S_un.S_addr = r->Data.A.IpAddress;
                wchar_t wide[32];
                MultiByteToWideChar(CP_ACP, 0, inet_ntoa(a), -1, wide, 32);
                swprintf(line, 512, L"%s has address %s", address, wide);
                break;
            }
            case DNS_TYPE_AAAA: {
                struct sockaddr_in6 sa6;
                ZeroMemory(&sa6, sizeof(sa6));
                sa6.sin6_family = AF_INET6;
                memcpy(&sa6.sin6_addr, &r->Data.AAAA.Ip6Address, sizeof(sa6.sin6_addr));
                wchar_t wide[64];
                DWORD wideLen = 64;
                WSAAddressToStringW((LPSOCKADDR)&sa6, sizeof(sa6), NULL, wide, &wideLen);
                swprintf(line, 512, L"%s has IPv6 address %s", address, wide);
                break;
            }
            case DNS_TYPE_MX:
                swprintf(line, 512, L"%s mail is handled by %u %s", address, r->Data.MX.wPreference,
                         r->Data.MX.pNameExchange);
                break;
            case DNS_TYPE_NS:
                swprintf(line, 512, L"%s name server %s", address, r->Data.NS.pNameHost);
                break;
            case DNS_TYPE_CNAME:
                swprintf(line, 512, L"%s is an alias for %s", address, r->Data.CNAME.pNameHost);
                break;
            case DNS_TYPE_TEXT: {
                line[0] = L'\0';
                for (DWORD i = 0; i < r->Data.TXT.dwStringCount; i++) {
                    wcsncat(line, r->Data.TXT.pStringArray[i], 400);
                }
                break;
            }
            case DNS_TYPE_SOA:
                swprintf(line, 512, L"%s SOA primary=%s admin=%s serial=%lu", address,
                         r->Data.SOA.pNamePrimaryServer, r->Data.SOA.pNameAdministrator,
                         (unsigned long)r->Data.SOA.dwSerialNo);
                break;
            default:
                line[0] = L'\0';
                break;
        }
        if (line[0]) {
            OutputEdit_AppendLine(output, line);
        }
    }
    DnsRecordListFree(records, DnsFreeRecordList);
}

INT_PTR CALLBACK LookupTab_DialogProc(HWND hDlg, UINT msg, WPARAM wParam, LPARAM lParam) {
    LookupState *state = (LookupState *)GetWindowLongPtrW(hDlg, DWLP_USER);

    switch (msg) {
        case WM_INITDIALOG: {
            state = (LookupState *)calloc(1, sizeof(LookupState));
            SetWindowLongPtrW(hDlg, DWLP_USER, (LONG_PTR)state);
            SetDlgItemTextW(hDlg, IDC_LOOKUP_LABEL_ADDR, T(STR_LOOKUP_LABEL));
            SetDlgItemTextW(hDlg, IDC_LOOKUP_LABEL_TYPE, T(STR_LOOKUP_TYPELABEL));
            SetDlgItemTextW(hDlg, IDC_LOOKUP_BTN, T(STR_LOOKUP_BTN));

            HWND combo = GetDlgItem(hDlg, IDC_LOOKUP_TYPE);
            for (int i = 0; i < TYPE_COUNT; i++) {
                int idx = ComboBox_AddString(combo, T(kTypes[i].label));
                ComboBox_SetItemData(combo, idx, kTypes[i].dnsType);
            }
            ComboBox_SetCurSel(combo, 0);
            state->resizer = DialogResize_Init(hDlg, kAnchors, (int)(sizeof(kAnchors) / sizeof(kAnchors[0])));
            return TRUE;
        }

        case WM_SIZE:
            if (state) {
                DialogResize_Apply(state->resizer, LOWORD(lParam), HIWORD(lParam));
            }
            return TRUE;

        case WM_COMMAND:
            if (LOWORD(wParam) == IDC_LOOKUP_BTN) {
                wchar_t address[256];
                GetDlgItemTextW(hDlg, IDC_LOOKUP_ADDR, address, 256);
                HWND output = GetDlgItem(hDlg, IDC_LOOKUP_OUTPUT);
                OutputEdit_Clear(output);
                if (wcslen(address) == 0) {
                    OutputEdit_AppendLine(output, T(STR_LOOKUP_NOADDR));
                    return TRUE;
                }

                HWND combo = GetDlgItem(hDlg, IDC_LOOKUP_TYPE);
                int sel = ComboBox_GetCurSel(combo);
                WORD type = sel >= 0 ? (WORD)ComboBox_GetItemData(combo, sel) : 0;

                if (type == 0) {
                    RunDefaultLookup(output, address);
                } else {
                    RunDnsQuery(output, address, type);
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
