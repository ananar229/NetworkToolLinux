#pragma once

#include <windows.h>

/* Appends text (as-is, no extra newline) to a read-only multiline edit
 * control and scrolls it into view - the Win32 equivalent of the Linux
 * app's OutputConsole. */
void OutputEdit_Append(HWND edit, const wchar_t *text);
void OutputEdit_AppendLine(HWND edit, const wchar_t *text);
void OutputEdit_Clear(HWND edit);
