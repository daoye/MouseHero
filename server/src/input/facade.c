#include "input/facade.h"
#include "ya_logger.h"

#ifdef USE_UINPUT
#include "input/backend/linux_uinput.h"
#endif

// Mouse button mapping (service owns this per FR-011)
#ifdef USE_UINPUT
static mouse_button_t map_button_to_backend(enum CButton btn) {
    switch (btn) {
        case Left:
            return MOUSE_BUTTON_LEFT;
        case Right:
            return MOUSE_BUTTON_RIGHT;
        case Middle:
            return MOUSE_BUTTON_MIDDLE;
        default:
            return -1;
    }
}

static scroll_direction_t map_scroll_dir_to_backend(int dir) {
    switch (dir) {
        case 0: return SCROLL_UP;
        case 1: return SCROLL_DOWN;
        case 2: return SCROLL_LEFT;
        case 3: return SCROLL_RIGHT;
        default: return -1;
    }
}

// Translate backend status to YAError
static YAError translate_backend_status(int status) {
    switch (status) {
        case INPUT_BACKEND_OK:
            return Success;
        case INPUT_BACKEND_ERROR_INVALID_PARAM:
            return InvalidInput;
        default:
            return PlatformError;
    }
}
#endif

YAError input_mouse_move(int dx, int dy) {
#ifdef USE_UINPUT
    int result = input_linux_mouse_move(dx, dy);
    if (result != INPUT_BACKEND_OK) {
        YA_LOG_ERROR("input_mouse_move failed: backend error %d", result);
    }
    return translate_backend_status(result);
#else
    return move_mouse(dx, dy, Rel);
#endif
}

// ============================================================================
// EXTENSION POINT: Mouse Button Behaviors
// ============================================================================
// To add custom button behaviors (e.g., double-click detection, gesture
// recognition), modify this function. The service layer isolates handlers
// from platform specifics, so changes here don't affect ya_server_handler.c.
// ============================================================================

YAError input_mouse_button(enum CButton btn, enum CDirection dir) {
#ifdef USE_UINPUT
    mouse_button_t backend_btn = map_button_to_backend(btn);
    if (backend_btn < 0) {
        YA_LOG_ERROR("input_mouse_button: unsupported button %d", (int)btn);
        return InvalidInput;
    }
    
    // Example extension: Double-click detection (behind dev flag)
    // Uncomment and implement if needed:
    // #ifdef ENABLE_DOUBLE_CLICK_DETECTION
    //     if (detect_double_click(btn, dir)) {
    //         // Handle double-click
    //     }
    // #endif
    
    int result;
    if (dir == Press) {
        result = input_linux_mouse_press(backend_btn);
    } else if (dir == Release) {
        result = input_linux_mouse_release(backend_btn);
    } else { // Click
        result = input_linux_mouse_click(backend_btn);
    }
    
    if (result != INPUT_BACKEND_OK) {
        YA_LOG_ERROR("input_mouse_button failed: backend error %d", result);
    }
    return translate_backend_status(result);
#else
    return mouse_button(btn, dir);
#endif
}

YAError input_mouse_scroll(int amount, int dir) {
#ifdef USE_UINPUT
    if (amount <= 0) {
        YA_LOG_ERROR("input_mouse_scroll: invalid amount %d", amount);
        return InvalidInput;
    }
    
    scroll_direction_t scroll_dir = map_scroll_dir_to_backend(dir);
    if (scroll_dir < 0) {
        YA_LOG_ERROR("input_mouse_scroll: invalid direction %d", dir);
        return InvalidInput;
    }
    
    int result = input_linux_mouse_scroll(scroll_dir, amount);
    if (result != INPUT_BACKEND_OK) {
        YA_LOG_ERROR("input_mouse_scroll failed: backend error %d", result);
    }
    return translate_backend_status(result);
#else
    // Non-Linux: use RS scroll buttons
    enum CButton wheel_btn;
    switch (dir) {
        case 0: wheel_btn = ScrollUp; break;
        case 1: wheel_btn = ScrollDown; break;
        case 2: wheel_btn = ScrollLeft; break;
        case 3: wheel_btn = ScrollRight; break;
        default:
            YA_LOG_ERROR("input_mouse_scroll: invalid direction %d", dir);
            return InvalidInput;
    }
    
    for (int i = 0; i < amount; ++i) {
        YAError err = mouse_button(wheel_btn, Click);
        if (err != Success) {
            YA_LOG_ERROR("input_mouse_scroll failed at step %d/%d: %d", i + 1, amount, err);
            return err;
        }
    }
    return Success;
#endif
}

YAError input_key_action(enum CKey key, enum CDirection dir) {
#ifdef USE_UINPUT
    int result = input_linux_key_action(key, dir);
    if (result != INPUT_BACKEND_OK) {
        YA_LOG_ERROR("input_key_action failed: backend error %d", result);
    }
    return translate_backend_status(result);
#else
    return key_action(key, dir);
#endif
}
