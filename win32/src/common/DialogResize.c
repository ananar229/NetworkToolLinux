#include "DialogResize.h"

#include <stdlib.h>

typedef struct {
    HWND hControl;
    RECT origRect; /* client-relative, at the dialog's as-authored size */
    BYTE anchors;
} ResizeItem;

struct DialogResizer {
    HWND hDlg;
    int origWidth;
    int origHeight;
    int count;
    ResizeItem items[1]; /* actually `count` entries, allocated to fit */
};

DialogResizer *DialogResize_Init(HWND hDlg, const ResizeAnchor *anchors, int count) {
    const size_t extra = count > 0 ? (size_t)(count - 1) * sizeof(ResizeItem) : 0;
    DialogResizer *resizer = (DialogResizer *)malloc(sizeof(DialogResizer) + extra);
    if (!resizer) {
        return NULL;
    }

    resizer->hDlg = hDlg;
    RECT client;
    GetClientRect(hDlg, &client);
    resizer->origWidth = client.right - client.left;
    resizer->origHeight = client.bottom - client.top;
    resizer->count = count;

    for (int i = 0; i < count; i++) {
        HWND hControl = GetDlgItem(hDlg, anchors[i].controlId);
        RECT rc = {0, 0, 0, 0};
        if (hControl) {
            GetWindowRect(hControl, &rc);
            MapWindowPoints(HWND_DESKTOP, hDlg, (LPPOINT)&rc, 2);
        }
        resizer->items[i].hControl = hControl;
        resizer->items[i].origRect = rc;
        resizer->items[i].anchors = anchors[i].anchors;
    }
    return resizer;
}

void DialogResize_Apply(DialogResizer *resizer, int width, int height) {
    if (!resizer) {
        return;
    }
    const int dx = width - resizer->origWidth;
    const int dy = height - resizer->origHeight;

    for (int i = 0; i < resizer->count; i++) {
        ResizeItem *item = &resizer->items[i];
        if (!item->hControl) {
            continue;
        }
        const BYTE a = item->anchors;
        const int left = item->origRect.left;
        const int top = item->origRect.top;
        const int w = item->origRect.right - item->origRect.left;
        const int h = item->origRect.bottom - item->origRect.top;

        const BOOL anchorLeft = (a & RESIZE_LEFT) != 0;
        const BOOL anchorRight = (a & RESIZE_RIGHT) != 0;
        const BOOL anchorTop = (a & RESIZE_TOP) != 0;
        const BOOL anchorBottom = (a & RESIZE_BOTTOM) != 0;

        int newLeft = left, newWidth = w;
        if (anchorLeft && anchorRight) {
            newWidth = w + dx;
        } else if (anchorRight) {
            newLeft = left + dx;
        }

        int newTop = top, newHeight = h;
        if (anchorTop && anchorBottom) {
            newHeight = h + dy;
        } else if (anchorBottom) {
            newTop = top + dy;
        }

        if (newWidth < 0) {
            newWidth = 0;
        }
        if (newHeight < 0) {
            newHeight = 0;
        }

        MoveWindow(item->hControl, newLeft, newTop, newWidth, newHeight, TRUE);

        /* bRepaint=TRUE above isn't always enough under Wine for a
         * *moved/reflowed* control to fully repaint at its new geometry:
         * push buttons can leave stale pixels at their old position, and
         * word-wrapped static labels (like the Port Scan warning) can
         * leave a stale fragment of their previous line-wrapping. Force
         * it explicitly for those two cases. Groupboxes are also class
         * "Button" (BS_GROUPBOX) but must NOT be force-erased this way:
         * their bounds visually span sibling controls that sit "inside"
         * them, and an explicit erase paints over those siblings without
         * them getting a chance to repaint themselves afterward. Other
         * control classes (edits, comboboxes, list views...) already
         * repaint correctly via MoveWindow alone. */
        wchar_t className[32];
        const LONG_PTR style = GetWindowLongPtrW(item->hControl, GWL_STYLE);
        const BOOL isGroupBox = (style & 0x0F) == BS_GROUPBOX;
        if (!isGroupBox && GetClassNameW(item->hControl, className, 32) &&
            (lstrcmpiW(className, L"Button") == 0 || lstrcmpiW(className, L"Static") == 0)) {
            InvalidateRect(item->hControl, NULL, TRUE);
            UpdateWindow(item->hControl);
        }
    }
}

void DialogResize_Free(DialogResizer *resizer) { free(resizer); }
