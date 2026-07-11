#pragma once
#include <windows.h>

/* Modal "About" dialog (Hilfe > Über): shows the app name/version, the full
 * MIT license text and a disclaimer. */
INT_PTR CALLBACK AboutDialog_Proc(HWND hDlg, UINT msg, WPARAM wParam, LPARAM lParam);
