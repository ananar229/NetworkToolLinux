#include "FingerTab.h"

#include <stdlib.h>
#include <wchar.h>

#include "common/DialogResize.h"
#include "common/OutputEdit.h"
#include "common/TcpQuery.h"
#include "common/Translations.h"
#include "common/resource.h"

typedef struct {
    DialogResizer *resizer;
} FingerState;

static const ResizeAnchor kAnchors[] = {
    {IDC_FINGER_ADDR, RESIZE_LEFT | RESIZE_TOP | RESIZE_RIGHT},
    {IDC_FINGER_BTN, RESIZE_RIGHT | RESIZE_TOP},
    {IDC_FINGER_OUTPUT, RESIZE_LEFT | RESIZE_TOP | RESIZE_RIGHT | RESIZE_BOTTOM},
};

INT_PTR CALLBACK FingerTab_DialogProc(HWND hDlg, UINT msg, WPARAM wParam, LPARAM lParam) {
    FingerState *state = (FingerState *)GetWindowLongPtrW(hDlg, DWLP_USER);

    switch (msg) {
        case WM_INITDIALOG:
            state = (FingerState *)calloc(1, sizeof(FingerState));
            SetWindowLongPtrW(hDlg, DWLP_USER, (LONG_PTR)state);
            SetDlgItemTextW(hDlg, IDC_FINGER_LABEL_ADDR, T(STR_FINGER_LABEL));
            SetDlgItemTextW(hDlg, IDC_FINGER_BTN, T(STR_FINGER_BTN));
            state->resizer = DialogResize_Init(hDlg, kAnchors, (int)(sizeof(kAnchors) / sizeof(kAnchors[0])));
            return TRUE;

        case WM_SIZE:
            if (state) {
                DialogResize_Apply(state->resizer, LOWORD(lParam), HIWORD(lParam));
            }
            return TRUE;

        case WM_COMMAND:
            if (LOWORD(wParam) == IDC_FINGER_BTN) {
                wchar_t input[256];
                GetDlgItemTextW(hDlg, IDC_FINGER_ADDR, input, 256);
                HWND output = GetDlgItem(hDlg, IDC_FINGER_OUTPUT);
                OutputEdit_Clear(output);
                if (wcslen(input) == 0) {
                    OutputEdit_AppendLine(output, T(STR_FINGER_NOADDR));
                    return TRUE;
                }

                wchar_t *at = wcschr(input, L'@');
                wchar_t user[128] = L"";
                const wchar_t *host;
                if (at) {
                    size_t userLen = at - input;
                    if (userLen >= 128) {
                        userLen = 127;
                    }
                    wcsncpy(user, input, userLen);
                    user[userLen] = L'\0';
                    host = at + 1;
                } else {
                    host = input;
                }
                if (wcslen(host) == 0) {
                    OutputEdit_AppendLine(output, T(STR_FINGER_NOHOST));
                    return TRUE;
                }

                wchar_t notice[300];
                swprintf(notice, 300, T(STR_FINGER_CONNECTING), host);
                wcscat(notice, L"\r\n");
                OutputEdit_Append(output, notice);

                wchar_t *response = NULL;
                wchar_t errorMsg[256];
                if (TcpQuery_SendReceive(host, 79, user, &response, errorMsg, 256)) {
                    OutputEdit_Append(output, response);
                    free(response);
                } else {
                    OutputEdit_AppendLine(output, errorMsg);
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
