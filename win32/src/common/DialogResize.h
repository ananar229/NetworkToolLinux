#pragma once
#include <windows.h>

/* Anchor an edge to keep its distance from the corresponding dialog edge
 * constant. LEFT+RIGHT (or TOP+BOTTOM) together means "stretch with the
 * dialog"; only one of the pair means "keep size, track that edge";
 * neither means "keep position and size fixed" (the default DS_CONTROL
 * behavior, since these tab pages are otherwise not resize-aware at all). */
#define RESIZE_LEFT 0x1
#define RESIZE_TOP 0x2
#define RESIZE_RIGHT 0x4
#define RESIZE_BOTTOM 0x8

typedef struct {
    int controlId;
    BYTE anchors;
} ResizeAnchor;

typedef struct DialogResizer DialogResizer;

/* Call once from WM_INITDIALOG, after all controls have been created and
 * before the page is ever resized - this relies on the dialog still being
 * at its as-authored template size at that point (CreateDialogParamW
 * guarantees this), which becomes the resize baseline. */
DialogResizer *DialogResize_Init(HWND hDlg, const ResizeAnchor *anchors, int count);

/* Call from WM_SIZE with the new client width/height (LOWORD/HIWORD of
 * lParam). */
void DialogResize_Apply(DialogResizer *resizer, int width, int height);

/* Call from WM_DESTROY. */
void DialogResize_Free(DialogResizer *resizer);
