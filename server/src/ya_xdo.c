#include "ya_xdo.h"

#ifdef __linux__
#include <xdo.h>
#include <stdlib.h>
#include <errno.h>

// Simple singleton for xdo handle
static xdo_t *g_xdo = NULL;

static int ensure_xdo() {
    if (g_xdo != NULL) return 0;
    // NULL uses $DISPLAY
    g_xdo = xdo_new(NULL);
    return (g_xdo != NULL) ? 0 : -1;
}

enum YAError ya_xdo_move_mouse(int32_t x, int32_t y, enum CCoordinate coord) {
    if (ensure_xdo() != 0) {
        return PlatformError;
    }

    int rc = 0;
    if (coord == Rel) {
        rc = xdo_move_mouse_relative(g_xdo, x, y);
    } else { // Abs
        // screen = CURRENT; use 0 which xdo treats as default
        rc = xdo_move_mouse(g_xdo, x, y, 0);
    }

    if (rc != 0) {
        return PlatformError;
    }
    return Success;
}

static int map_button(enum CButton b) {
    switch (b) {
        case Left:    return 1;
        case Middle:  return 2;
        case Right:   return 3;
        case ScrollUp:    return 4;
        case ScrollDown:  return 5;
        case ScrollLeft:  return 6;
        case ScrollRight: return 7;
        default: return 1;
    }
}

enum YAError ya_xdo_button(enum CButton button, enum CDirection dir) {
    if (ensure_xdo() != 0) {
        return PlatformError;
    }
    int xb = map_button(button);
    int rc = 0;
    switch (dir) {
        case Press:
            rc = xdo_mouse_down(g_xdo, CURRENTWINDOW, xb);
            break;
        case Release:
            rc = xdo_mouse_up(g_xdo, CURRENTWINDOW, xb);
            break;
        case Click:
            rc = xdo_click_window(g_xdo, CURRENTWINDOW, xb);
            break;
        default:
            rc = -1;
            break;
    }
    return (rc == 0) ? Success : PlatformError;
}

#endif // __linux__
