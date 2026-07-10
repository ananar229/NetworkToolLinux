#include "OutputEdit.h"

void OutputEdit_Append(HWND edit, const wchar_t *text) {
    int len = GetWindowTextLengthW(edit);
    SendMessageW(edit, EM_SETSEL, (WPARAM)len, (LPARAM)len);
    SendMessageW(edit, EM_REPLACESEL, FALSE, (LPARAM)text);
    SendMessageW(edit, EM_SCROLLCARET, 0, 0);
}

void OutputEdit_AppendLine(HWND edit, const wchar_t *text) {
    OutputEdit_Append(edit, text);
    OutputEdit_Append(edit, L"\r\n");
}

void OutputEdit_Clear(HWND edit) {
    SetWindowTextW(edit, L"");
}
