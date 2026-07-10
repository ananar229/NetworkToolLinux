#pragma once

#include <windows.h>

/* Registers the main window class, creates the window (tab control +
 * one child dialog per tool tab) and shows it. Returns the HWND, or
 * NULL on failure. */
HWND MainWindow_Create(HINSTANCE hInstance, int nCmdShow);

/* Returns the currently visible tab page (a dialog HWND), so the message
 * loop can route keyboard navigation to it via IsDialogMessage. */
HWND MainWindow_GetActivePage(HWND hMainWnd);

/* Global accelerator table (Ctrl+N, Ctrl+W, Ctrl+Z, ...), loaded once and
 * shared by every top-level window; the message loop passes it (together
 * with the message's own top-level window) to TranslateAccelerator. */
HACCEL MainWindow_GetAcceleratorTable(void);
