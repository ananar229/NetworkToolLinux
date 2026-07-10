#pragma once
#include <windows.h>

/* Modal "Settings" dialog (Hilfe > Einstellungen): lets the user pick a
 * UI language. Changing it saves the choice and, after confirmation,
 * relaunches the application (the same "restart required" flow the Linux
 * build uses, since there's no live re-translation of already-created
 * dialogs here). */
INT_PTR CALLBACK SettingsDialog_Proc(HWND hDlg, UINT msg, WPARAM wParam, LPARAM lParam);
