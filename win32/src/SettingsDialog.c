#include "SettingsDialog.h"

#include <windowsx.h>

#include "common/Translations.h"
#include "common/resource.h"

static void RelaunchApplication(void) {
    wchar_t exePath[MAX_PATH];
    GetModuleFileNameW(NULL, exePath, MAX_PATH);

    STARTUPINFOW si;
    PROCESS_INFORMATION pi;
    ZeroMemory(&si, sizeof(si));
    si.cb = sizeof(si);
    ZeroMemory(&pi, sizeof(pi));

    if (CreateProcessW(exePath, NULL, NULL, NULL, FALSE, 0, NULL, NULL, &si, &pi)) {
        CloseHandle(pi.hProcess);
        CloseHandle(pi.hThread);
    }
}

static int CurrentSavedOptionIndex(void) {
    wchar_t savedCode[16];
    Translations_GetSavedLanguageCode(savedCode, 16);
    for (int i = 0; i < Translations_OptionCount(); i++) {
        if (_wcsicmp(Translations_OptionCode(i), savedCode) == 0) {
            return i;
        }
    }
    return 0;
}

INT_PTR CALLBACK SettingsDialog_Proc(HWND hDlg, UINT msg, WPARAM wParam, LPARAM lParam) {
    switch (msg) {
        case WM_INITDIALOG: {
            SetWindowTextW(hDlg, T(STR_SETTINGS_TITLE));
            SetDlgItemTextW(hDlg, IDC_SETTINGS_LABEL_LANGUAGE, T(STR_SETTINGS_LANGUAGE));
            SetDlgItemTextW(hDlg, IDC_SETTINGS_CLOSE, T(STR_SETTINGS_CLOSE));

            HWND combo = GetDlgItem(hDlg, IDC_SETTINGS_LANG_COMBO);
            for (int i = 0; i < Translations_OptionCount(); i++) {
                ComboBox_AddString(combo, Translations_OptionNativeName(i));
            }
            ComboBox_SetCurSel(combo, CurrentSavedOptionIndex());
            return TRUE;
        }

        case WM_COMMAND:
            if (LOWORD(wParam) == IDC_SETTINGS_LANG_COMBO && HIWORD(wParam) == CBN_SELCHANGE) {
                HWND combo = GetDlgItem(hDlg, IDC_SETTINGS_LANG_COMBO);
                int index = ComboBox_GetCurSel(combo);
                if (index == CB_ERR || index == CurrentSavedOptionIndex()) {
                    return TRUE;
                }
                const wchar_t *newCode = Translations_OptionCode(index);
                Translations_SaveLanguageCode(newCode);

                int reply = MessageBoxW(hDlg, T(STR_SETTINGS_RESTART_BODY), T(STR_SETTINGS_RESTART_TITLE),
                                         MB_YESNO | MB_ICONQUESTION);
                if (reply == IDYES) {
                    RelaunchApplication();
                    EndDialog(hDlg, IDOK);
                    PostQuitMessage(0);
                }
                return TRUE;
            }
            if (LOWORD(wParam) == IDC_SETTINGS_CLOSE || LOWORD(wParam) == IDCANCEL) {
                EndDialog(hDlg, IDCANCEL);
                return TRUE;
            }
            return TRUE;

        default:
            break;
    }
    (void)lParam;
    return FALSE;
}
