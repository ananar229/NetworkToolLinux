#include <windows.h>
#include <winsock2.h>

#include "MainWindow.h"
#include "common/Translations.h"

int WINAPI wWinMain(HINSTANCE hInstance, HINSTANCE hPrevInstance, PWSTR pCmdLine, int nCmdShow) {
    (void)hPrevInstance;
    (void)pCmdLine;

    WSADATA wsaData;
    WSAStartup(MAKEWORD(2, 2), &wsaData);

    Translations_Init();

    HWND hWnd = MainWindow_Create(hInstance, nCmdShow);
    if (!hWnd) {
        WSACleanup();
        return 1;
    }

    MSG msg;
    while (GetMessageW(&msg, NULL, 0, 0) > 0) {
        HWND hTopLevel = msg.hwnd ? GetAncestor(msg.hwnd, GA_ROOT) : NULL;

        HACCEL hAccel = MainWindow_GetAcceleratorTable();
        if (hTopLevel && hAccel && TranslateAcceleratorW(hTopLevel, hAccel, &msg)) {
            continue;
        }

        HWND hActivePage = hTopLevel ? MainWindow_GetActivePage(hTopLevel) : NULL;
        if (!hActivePage || !IsDialogMessageW(hActivePage, &msg)) {
            TranslateMessage(&msg);
            DispatchMessageW(&msg);
        }
    }

    WSACleanup();
    return (int)msg.wParam;
}
