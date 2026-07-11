#include "AboutDialog.h"

#include "common/Translations.h"
#include "common/resource.h"

/* Legal text is kept in its canonical English form regardless of UI
 * language, matching the repository's root LICENSE file. Edit controls
 * need \r\n (not just \n) to break lines. */
static const wchar_t kMitLicenseText[] =
    L"MIT License\r\n"
    L"\r\n"
    L"Copyright (c) 2026 ananar229\r\n"
    L"\r\n"
    L"Permission is hereby granted, free of charge, to any person obtaining a copy of this "
    L"software and associated documentation files (the \"Software\"), to deal in the Software "
    L"without restriction, including without limitation the rights to use, copy, modify, merge, "
    L"publish, distribute, sublicense, and/or sell copies of the Software, and to permit persons "
    L"to whom the Software is furnished to do so, subject to the following conditions:\r\n"
    L"\r\n"
    L"The above copyright notice and this permission notice shall be included in all copies or "
    L"substantial portions of the Software.\r\n"
    L"\r\n"
    L"THE SOFTWARE IS PROVIDED \"AS IS\", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR IMPLIED, "
    L"INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY, FITNESS FOR A PARTICULAR "
    L"PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE "
    L"FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR "
    L"OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER "
    L"DEALINGS IN THE SOFTWARE.";

/* EM_LINESCROLL clamps at the top regardless of caret/format-rect state,
 * unlike EM_SCROLLCARET (which can be a no-op before the control has done
 * its first layout pass) - a large negative line count reliably rewinds
 * a multi-line edit control to its very first line. */
static void ScrollEditToTop(HWND hDlg, int controlId) {
    HWND hEdit = GetDlgItem(hDlg, controlId);
    SendMessageW(hEdit, EM_LINESCROLL, 0, -1000000);
}

INT_PTR CALLBACK AboutDialog_Proc(HWND hDlg, UINT msg, WPARAM wParam, LPARAM lParam) {
    switch (msg) {
        case WM_INITDIALOG: {
            SetWindowTextW(hDlg, T(STR_ABOUT_TITLE));
            SetDlgItemTextW(hDlg, IDC_ABOUT_TITLE, L"Network Tool");
            SetDlgItemTextW(hDlg, IDC_ABOUT_VERSION, T(STR_ABOUT_VERSION));
            SetDlgItemTextW(hDlg, IDC_ABOUT_LICENSE_LABEL, T(STR_ABOUT_LICENSE));
            SetDlgItemTextW(hDlg, IDC_ABOUT_LICENSE_TEXT, kMitLicenseText);
            SetDlgItemTextW(hDlg, IDC_ABOUT_DISCLAIMER_TITLE, T(STR_ABOUT_DISCLAIMER_TITLE));
            SetDlgItemTextW(hDlg, IDC_ABOUT_DISCLAIMER_BODY, T(STR_ABOUT_DISCLAIMER_BODY));
            SetDlgItemTextW(hDlg, IDC_ABOUT_CLOSE, T(STR_SETTINGS_CLOSE));

            ScrollEditToTop(hDlg, IDC_ABOUT_LICENSE_TEXT);
            ScrollEditToTop(hDlg, IDC_ABOUT_DISCLAIMER_BODY);
            SetFocus(GetDlgItem(hDlg, IDC_ABOUT_CLOSE));
            return FALSE;
        }

        case WM_SHOWWINDOW:
            if (wParam) {
                ScrollEditToTop(hDlg, IDC_ABOUT_LICENSE_TEXT);
                ScrollEditToTop(hDlg, IDC_ABOUT_DISCLAIMER_BODY);
            }
            return FALSE;

        case WM_COMMAND:
            if (LOWORD(wParam) == IDC_ABOUT_CLOSE || LOWORD(wParam) == IDCANCEL) {
                EndDialog(hDlg, IDOK);
                return TRUE;
            }
            return TRUE;

        default:
            break;
    }
    (void)lParam;
    return FALSE;
}
