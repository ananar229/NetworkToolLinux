#include "MainWindow.h"

#include <commctrl.h>
#include <stdio.h>
#include <stdlib.h>
#include "AboutDialog.h"
#include "SettingsDialog.h"
#include "common/Translations.h"
#include "common/resource.h"
#include "tabs/FingerTab.h"
#include "tabs/InfoTab.h"
#include "tabs/LookupTab.h"
#include "tabs/NetstatTab.h"
#include "tabs/PingTab.h"
#include "tabs/PortScanTab.h"
#include "tabs/SpeedTab.h"
#include "tabs/TracerouteTab.h"
#include "tabs/WhoisTab.h"

#define WINDOW_CLASS_NAME L"NetworkToolMainWindow"

typedef struct {
    const wchar_t *label;
    int dialogId;
    DLGPROC proc;
} TabSpec;

static const TabSpec kTabs[] = {
    {L"Info", IDD_INFO, InfoTab_DialogProc},
    {L"Netstat", IDD_NETSTAT, NetstatTab_DialogProc},
    {L"Ping", IDD_PING, PingTab_DialogProc},
    {L"Lookup", IDD_LOOKUP, LookupTab_DialogProc},
    {L"Traceroute", IDD_TRACEROUTE, TracerouteTab_DialogProc},
    {L"Whois", IDD_WHOIS, WhoisTab_DialogProc},
    {L"Finger", IDD_FINGER, FingerTab_DialogProc},
    {L"Port Scan", IDD_PORTSCAN, PortScanTab_DialogProc},
    {L"Speed", IDD_SPEED, SpeedTab_DialogProc},
};
#define TAB_COUNT (int)(sizeof(kTabs) / sizeof(kTabs[0]))

typedef struct {
    HWND hTabControl;
    HWND hPages[TAB_COUNT];
    int currentTab;
    BOOL isFullscreen;
    DWORD savedStyle;
    RECT savedRect;
} MainWindowState;

static HACCEL g_hAccel = NULL;
static LONG g_windowCount = 0;

/* The tab control spans the full client width and uses TCS_RIGHTJUSTIFY
 * (set at creation) so all tabs are evenly stretched to fill that width
 * instead of being left-packed with empty space on the right - this is
 * the built-in, standard tab control layout mode, so it never needs
 * manual width measurement/centering and never runs into the clipped/
 * scroll-arrow state a too-narrow custom width could cause. */
static void LayoutChildren(HWND hWnd, MainWindowState *state) {
    RECT client;
    GetClientRect(hWnd, &client);
    int clientWidth = client.right - client.left;
    int clientHeight = client.bottom - client.top;

    MoveWindow(state->hTabControl, 0, 0, clientWidth, clientHeight, TRUE);

    RECT display = client;
    TabCtrl_AdjustRect(state->hTabControl, FALSE, &display);

    for (int i = 0; i < TAB_COUNT; i++) {
        if (state->hPages[i]) {
            MoveWindow(state->hPages[i], display.left, display.top, display.right - display.left,
                       display.bottom - display.top, TRUE);
        }
    }
}

static void SelectTab(MainWindowState *state, int index) {
    if (index == state->currentTab || index < 0 || index >= TAB_COUNT) {
        return;
    }
    if (state->currentTab >= 0 && state->hPages[state->currentTab]) {
        ShowWindow(state->hPages[state->currentTab], SW_HIDE);
    }
    state->currentTab = index;
    if (state->hPages[index]) {
        ShowWindow(state->hPages[index], SW_SHOW);
        /* SetFocus on the page itself does not reliably move keyboard focus
         * into one of its controls (it is a plain DS_CONTROL child, not a
         * "real" focusable control) - explicitly focus its first tabstop
         * control instead, exactly like WM_INITDIALOG's default handling
         * does when a dialog is first created. Leaving focus dangling on a
         * control from a different, now-hidden page was the cause of stray
         * keystrokes (including literal Tab characters) landing in the
         * wrong, invisible edit box. */
        HWND firstControl = GetNextDlgTabItem(state->hPages[index], NULL, FALSE);
        SetFocus(firstControl ? firstControl : state->hPages[index]);
    }
}

static void InvokeEditCommand(WORD cmd) {
    HWND focus = GetFocus();
    if (!focus) {
        return;
    }
    wchar_t className[32];
    GetClassNameW(focus, className, 32);
    if (lstrcmpiW(className, L"Edit") != 0) {
        return;
    }
    switch (cmd) {
        case IDM_EDIT_UNDO:
        case IDM_EDIT_REDO:
            SendMessageW(focus, EM_UNDO, 0, 0);
            break;
        case IDM_EDIT_CUT:
            SendMessageW(focus, WM_CUT, 0, 0);
            break;
        case IDM_EDIT_COPY:
            SendMessageW(focus, WM_COPY, 0, 0);
            break;
        case IDM_EDIT_PASTE:
            SendMessageW(focus, WM_PASTE, 0, 0);
            break;
        case IDM_EDIT_DELETE:
            SendMessageW(focus, WM_CLEAR, 0, 0);
            break;
        case IDM_EDIT_SELECTALL:
            SendMessageW(focus, EM_SETSEL, 0, -1);
            break;
        default:
            break;
    }
}

static void ToggleFullscreen(HWND hWnd, MainWindowState *state) {
    if (!state->isFullscreen) {
        GetWindowRect(hWnd, &state->savedRect);
        state->savedStyle = (DWORD)GetWindowLongPtrW(hWnd, GWL_STYLE);
        SetWindowLongPtrW(hWnd, GWL_STYLE, state->savedStyle & ~(WS_CAPTION | WS_THICKFRAME));

        MONITORINFO mi;
        mi.cbSize = sizeof(mi);
        GetMonitorInfoW(MonitorFromWindow(hWnd, MONITOR_DEFAULTTONEAREST), &mi);
        SetWindowPos(hWnd, HWND_TOP, mi.rcMonitor.left, mi.rcMonitor.top,
                     mi.rcMonitor.right - mi.rcMonitor.left, mi.rcMonitor.bottom - mi.rcMonitor.top,
                     SWP_NOZORDER | SWP_FRAMECHANGED);
        state->isFullscreen = TRUE;
    } else {
        SetWindowLongPtrW(hWnd, GWL_STYLE, state->savedStyle);
        SetWindowPos(hWnd, HWND_TOP, state->savedRect.left, state->savedRect.top,
                     state->savedRect.right - state->savedRect.left,
                     state->savedRect.bottom - state->savedRect.top, SWP_NOZORDER | SWP_FRAMECHANGED);
        state->isFullscreen = FALSE;
    }
}

static void ShowHelp(HWND hWnd) {
    MessageBoxW(hWnd, T(STR_HELP_BODY), T(STR_HELP_TITLE), MB_OK | MB_ICONINFORMATION);
}

static void ShowAbout(HWND hWnd) {
    DialogBoxParamW((HINSTANCE)GetWindowLongPtrW(hWnd, GWLP_HINSTANCE), MAKEINTRESOURCEW(IDD_ABOUT), hWnd,
                     AboutDialog_Proc, 0);
}

/* Built at runtime (rather than a static .rc MENU resource) so its text
 * follows the currently selected UI language. */
static HMENU BuildMainMenu(void) {
    HMENU hMenu = CreateMenu();

    HMENU fileMenu = CreatePopupMenu();
    AppendMenuW(fileMenu, MF_STRING, IDM_FILE_NEW, T(STR_MENU_FILE_NEW));
    AppendMenuW(fileMenu, MF_STRING, IDM_FILE_CLOSE, T(STR_MENU_FILE_CLOSE));
    AppendMenuW(hMenu, MF_POPUP, (UINT_PTR)fileMenu, T(STR_MENU_FILE));

    HMENU editMenu = CreatePopupMenu();
    AppendMenuW(editMenu, MF_STRING, IDM_EDIT_UNDO, T(STR_MENU_EDIT_UNDO));
    AppendMenuW(editMenu, MF_STRING, IDM_EDIT_REDO, T(STR_MENU_EDIT_REDO));
    AppendMenuW(editMenu, MF_SEPARATOR, 0, NULL);
    AppendMenuW(editMenu, MF_STRING, IDM_EDIT_CUT, T(STR_MENU_EDIT_CUT));
    AppendMenuW(editMenu, MF_STRING, IDM_EDIT_COPY, T(STR_MENU_EDIT_COPY));
    AppendMenuW(editMenu, MF_STRING, IDM_EDIT_PASTE, T(STR_MENU_EDIT_PASTE));
    AppendMenuW(editMenu, MF_STRING, IDM_EDIT_DELETE, T(STR_MENU_EDIT_DELETE));
    AppendMenuW(editMenu, MF_STRING, IDM_EDIT_SELECTALL, T(STR_MENU_EDIT_SELECTALL));
    AppendMenuW(hMenu, MF_POPUP, (UINT_PTR)editMenu, T(STR_MENU_EDIT));

    HMENU viewMenu = CreatePopupMenu();
    AppendMenuW(viewMenu, MF_STRING, IDM_VIEW_FULLSCREEN, T(STR_MENU_VIEW_FULLSCREEN));
    AppendMenuW(hMenu, MF_POPUP, (UINT_PTR)viewMenu, T(STR_MENU_VIEW));

    HMENU helpMenu = CreatePopupMenu();
    AppendMenuW(helpMenu, MF_STRING, IDM_HELP_SETTINGS, T(STR_MENU_HELP_SETTINGS));
    AppendMenuW(helpMenu, MF_SEPARATOR, 0, NULL);
    AppendMenuW(helpMenu, MF_STRING, IDM_HELP_HELP, T(STR_MENU_HELP_HELP));
    AppendMenuW(helpMenu, MF_STRING, IDM_HELP_ABOUT, T(STR_MENU_HELP_ABOUT));
    AppendMenuW(hMenu, MF_POPUP, (UINT_PTR)helpMenu, T(STR_MENU_HELP));

    return hMenu;
}

static LRESULT CALLBACK MainWndProc(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam) {
    MainWindowState *state = (MainWindowState *)GetWindowLongPtrW(hWnd, GWLP_USERDATA);

    switch (msg) {
        case WM_CREATE: {
            state = (MainWindowState *)calloc(1, sizeof(MainWindowState));
            state->currentTab = -1;
            SetWindowLongPtrW(hWnd, GWLP_USERDATA, (LONG_PTR)state);

            HINSTANCE hInstance = ((LPCREATESTRUCTW)lParam)->hInstance;

            RECT initialClient;
            GetClientRect(hWnd, &initialClient);

            state->hTabControl = CreateWindowExW(
                0, WC_TABCONTROLW, L"", WS_CHILD | WS_VISIBLE | WS_CLIPSIBLINGS | TCS_RIGHTJUSTIFY, 0, 0,
                initialClient.right, initialClient.bottom, hWnd, (HMENU)(UINT_PTR)IDC_TABCONTROL, hInstance, NULL);
            SendMessageW(state->hTabControl, WM_SETFONT, (WPARAM)GetStockObject(DEFAULT_GUI_FONT), TRUE);

            for (int i = 0; i < TAB_COUNT; i++) {
                TCITEMW item;
                item.mask = TCIF_TEXT;
                item.pszText = (LPWSTR)kTabs[i].label;
                TabCtrl_InsertItem(state->hTabControl, i, &item);

                state->hPages[i] = CreateDialogParamW(hInstance, MAKEINTRESOURCEW(kTabs[i].dialogId), hWnd,
                                                       kTabs[i].proc, 0);
                if (state->hPages[i]) {
                    /* DIALOGEX templates intentionally omit WS_VISIBLE, but
                     * CreateDialogParam shows WS_CHILD dialogs anyway - force
                     * them hidden so only the selected tab is ever visible. */
                    ShowWindow(state->hPages[i], SW_HIDE);
                }
            }

            LayoutChildren(hWnd, state);
            SelectTab(state, 0);

            if (!g_hAccel) {
                g_hAccel = LoadAcceleratorsW(hInstance, MAKEINTRESOURCEW(IDR_ACCELERATORS));
            }
            InterlockedIncrement(&g_windowCount);
            return 0;
        }

        case WM_SIZE:
            if (state) {
                LayoutChildren(hWnd, state);
            }
            return 0;

        case WM_NOTIFY: {
            LPNMHDR hdr = (LPNMHDR)lParam;
            if (state && hdr->hwndFrom == state->hTabControl && hdr->code == TCN_SELCHANGE) {
                SelectTab(state, TabCtrl_GetCurSel(state->hTabControl));
            }
            return 0;
        }

        case WM_COMMAND:
            switch (LOWORD(wParam)) {
                case IDM_FILE_NEW:
                    MainWindow_Create((HINSTANCE)GetWindowLongPtrW(hWnd, GWLP_HINSTANCE), SW_SHOW);
                    return 0;
                case IDM_FILE_CLOSE:
                    PostMessageW(hWnd, WM_CLOSE, 0, 0);
                    return 0;
                case IDM_EDIT_UNDO:
                case IDM_EDIT_REDO:
                case IDM_EDIT_CUT:
                case IDM_EDIT_COPY:
                case IDM_EDIT_PASTE:
                case IDM_EDIT_DELETE:
                case IDM_EDIT_SELECTALL:
                    InvokeEditCommand(LOWORD(wParam));
                    return 0;
                case IDM_VIEW_FULLSCREEN:
                    if (state) {
                        ToggleFullscreen(hWnd, state);
                    }
                    return 0;
                case IDM_HELP_SETTINGS:
                    DialogBoxParamW((HINSTANCE)GetWindowLongPtrW(hWnd, GWLP_HINSTANCE), MAKEINTRESOURCEW(IDD_SETTINGS),
                                     hWnd, SettingsDialog_Proc, 0);
                    return 0;
                case IDM_HELP_HELP:
                    ShowHelp(hWnd);
                    return 0;
                case IDM_HELP_ABOUT:
                    ShowAbout(hWnd);
                    return 0;
                default:
                    break;
            }
            return 0;

        case WM_CTLCOLORSTATIC:
            /* Let read-only "value" static labels and group boxes use the
             * normal window background rather than the default gray. */
            SetBkMode((HDC)wParam, TRANSPARENT);
            return (LRESULT)GetSysColorBrush(COLOR_BTNFACE);

        case WM_DESTROY:
            if (state) {
                free(state);
                SetWindowLongPtrW(hWnd, GWLP_USERDATA, 0);
            }
            if (InterlockedDecrement(&g_windowCount) == 0) {
                PostQuitMessage(0);
            }
            return 0;

        default:
            break;
    }
    return DefWindowProcW(hWnd, msg, wParam, lParam);
}

HWND MainWindow_GetActivePage(HWND hMainWnd) {
    MainWindowState *state = (MainWindowState *)GetWindowLongPtrW(hMainWnd, GWLP_USERDATA);
    if (!state || state->currentTab < 0) {
        return NULL;
    }
    return state->hPages[state->currentTab];
}

HACCEL MainWindow_GetAcceleratorTable(void) {
    return g_hAccel;
}

HWND MainWindow_Create(HINSTANCE hInstance, int nCmdShow) {
    INITCOMMONCONTROLSEX icc;
    icc.dwSize = sizeof(icc);
    icc.dwICC = ICC_TAB_CLASSES | ICC_PROGRESS_CLASS | ICC_STANDARD_CLASSES;
    InitCommonControlsEx(&icc);

    HICON hIcon = LoadIconW(hInstance, MAKEINTRESOURCEW(IDI_APPICON));

    WNDCLASSEXW wc;
    ZeroMemory(&wc, sizeof(wc));
    wc.cbSize = sizeof(wc);
    if (!GetClassInfoExW(hInstance, WINDOW_CLASS_NAME, &wc)) {
        wc.cbSize = sizeof(wc);
        wc.style = CS_HREDRAW | CS_VREDRAW;
        wc.lpfnWndProc = MainWndProc;
        wc.hInstance = hInstance;
        wc.hIcon = hIcon;
        wc.hIconSm = hIcon;
        wc.hCursor = LoadCursorW(NULL, IDC_ARROW);
        wc.hbrBackground = (HBRUSH)(COLOR_BTNFACE + 1);
        wc.lpszClassName = WINDOW_CLASS_NAME;

        if (!RegisterClassExW(&wc)) {
            return NULL;
        }
    }

    HMENU hMenu = BuildMainMenu();

    HWND hWnd = CreateWindowExW(0, WINDOW_CLASS_NAME, L"Network Tool", WS_OVERLAPPEDWINDOW, CW_USEDEFAULT,
                                 CW_USEDEFAULT, 860, 620, NULL, hMenu, hInstance, NULL);
    if (!hWnd) {
        return NULL;
    }

    ShowWindow(hWnd, nCmdShow);
    UpdateWindow(hWnd);
    return hWnd;
}
