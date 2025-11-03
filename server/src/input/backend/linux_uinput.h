#ifndef INPUT_LINUX_UINPUT_H
#define INPUT_LINUX_UINPUT_H

#include <stdint.h>
#include <stdbool.h>

// Backend status codes
typedef enum {
    INPUT_BACKEND_OK = 0,
    INPUT_BACKEND_ERROR_INIT = -1,
    INPUT_BACKEND_ERROR_PERMISSION = -2,
    INPUT_BACKEND_ERROR_NOT_AVAILABLE = -3,
    INPUT_BACKEND_ERROR_INVALID_PARAM = -4,
    INPUT_BACKEND_ERROR_DEVICE = -5
} input_linux_status_t;

// Mouse button types
typedef enum {
    MOUSE_BUTTON_LEFT = 0,
    MOUSE_BUTTON_RIGHT = 1,
    MOUSE_BUTTON_MIDDLE = 2
} mouse_button_t;

// Mouse scroll direction types
typedef enum {
    SCROLL_UP = 0,
    SCROLL_DOWN = 1,
    SCROLL_LEFT = 2,
    SCROLL_RIGHT = 3
} scroll_direction_t;

// Diagnostic result structure
typedef struct {
    bool device_exists;
    bool has_write_permission;
    bool udev_rule_detected;
    bool non_root_verified;
    char error_message[256];
} input_linux_diagnose_t;

// Backend interface functions
int input_linux_init(void);
void input_linux_shutdown(void);
int input_linux_mouse_move(int32_t dx, int32_t dy);
int input_linux_mouse_click(mouse_button_t button);
int input_linux_mouse_press(mouse_button_t button);
int input_linux_mouse_release(mouse_button_t button);
int input_linux_mouse_scroll(scroll_direction_t direction, int32_t amount);
int input_linux_key_press(uint32_t keycode);
int input_linux_diagnose(input_linux_diagnose_t *result);

// High-level key action wrapper (for key_inject.c integration)
// Takes CKey enum and direction, returns 0 on success, negative on error
#ifdef USE_UINPUT
#include "rs.h"
int input_linux_key_action(enum CKey key, enum CDirection direction);

// Raw evdev keycode injection (for xkb mapping)
// Takes Linux evdev KEY_* code and direction, returns 0 on success, negative on error
int input_linux_key_action_raw(int evdev_key, enum CDirection direction);
#endif

#endif // INPUT_LINUX_UINPUT_H
