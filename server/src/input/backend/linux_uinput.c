#ifdef __linux__

#include "input/backend/linux_uinput.h"
#include "rs.h"
#include "ya_logger.h"
#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include <fcntl.h>
#include <errno.h>
#include <linux/uinput.h>
#include <linux/input-event-codes.h>

static int uinput_fd = -1;

// Helper to map high-level mouse button enum to Linux BTN_* code
static int button_to_linux_code(mouse_button_t button) {
    switch (button) {
        case MOUSE_BUTTON_LEFT:
            return BTN_LEFT;
        case MOUSE_BUTTON_RIGHT:
            return BTN_RIGHT;
        case MOUSE_BUTTON_MIDDLE:
            return BTN_MIDDLE;
        default:
            return -1;
    }
}

// Map CKey enum to Linux keycode
// Complete US keyboard mapping
static int ckey_to_linux_keycode(enum CKey key) {
    switch (key) {
        // Numbers (top row)
        case Num0: return KEY_0;
        case Num1: return KEY_1;
        case Num2: return KEY_2;
        case Num3: return KEY_3;
        case Num4: return KEY_4;
        case Num5: return KEY_5;
        case Num6: return KEY_6;
        case Num7: return KEY_7;
        case Num8: return KEY_8;
        case Num9: return KEY_9;
        
        // Letters
        case A: return KEY_A;
        case B: return KEY_B;
        case C: return KEY_C;
        case D: return KEY_D;
        case E: return KEY_E;
        case F: return KEY_F;
        case G: return KEY_G;
        case H: return KEY_H;
        case I: return KEY_I;
        case J: return KEY_J;
        case K: return KEY_K;
        case L: return KEY_L;
        case M: return KEY_M;
        case N: return KEY_N;
        case O: return KEY_O;
        case P: return KEY_P;
        case Q: return KEY_Q;
        case R: return KEY_R;
        case S: return KEY_S;
        case T: return KEY_T;
        case U: return KEY_U;
        case V: return KEY_V;
        case W: return KEY_W;
        case X: return KEY_X;
        case Y: return KEY_Y;
        case Z: return KEY_Z;
        
        // Modifiers - Left side
        case Alt:
        case LMenu: return KEY_LEFTALT;
        case Control:
        case LControl: return KEY_LEFTCTRL;
        case Shift:
        case LShift: return KEY_LEFTSHIFT;
        case Meta:
        case LWin:
        case Command: return KEY_LEFTMETA;
        
        // Modifiers - Right side
        case RMenu: return KEY_RIGHTALT;
        case RControl: return KEY_RIGHTCTRL;
        case RShift: return KEY_RIGHTSHIFT;
        case RWin:
        case RCommand:
        case ROption: return KEY_RIGHTMETA;
        case Option: return KEY_LEFTALT;  // Mac Option key
        
        // Function keys
        case F1: return KEY_F1;
        case F2: return KEY_F2;
        case F3: return KEY_F3;
        case F4: return KEY_F4;
        case F5: return KEY_F5;
        case F6: return KEY_F6;
        case F7: return KEY_F7;
        case F8: return KEY_F8;
        case F9: return KEY_F9;
        case F10: return KEY_F10;
        case F11: return KEY_F11;
        case F12: return KEY_F12;
        case F13: return KEY_F13;
        case F14: return KEY_F14;
        case F15: return KEY_F15;
        case F16: return KEY_F16;
        case F17: return KEY_F17;
        case F18: return KEY_F18;
        case F19: return KEY_F19;
        case F20: return KEY_F20;
        case F21: return KEY_F21;
        case F22: return KEY_F22;
        case F23: return KEY_F23;
        case F24: return KEY_F24;
        
        // Navigation - Arrow keys
        case UpArrow: return KEY_UP;
        case DownArrow: return KEY_DOWN;
        case LeftArrow: return KEY_LEFT;
        case RightArrow: return KEY_RIGHT;
        
        // Navigation - Page keys
        case Home: return KEY_HOME;
        case End: return KEY_END;
        case PageUp: return KEY_PAGEUP;
        case PageDown: return KEY_PAGEDOWN;
        
        // Editing keys
        case Escape_k: return KEY_ESC;
        case Return: return KEY_ENTER;
        case Space: return KEY_SPACE;
        case Tab: return KEY_TAB;
        case Backspace: return KEY_BACKSPACE;
        case Delete: return KEY_DELETE;
        case Insert: return KEY_INSERT;
        
        // Lock keys
        case CapsLock: return KEY_CAPSLOCK;
        case Numlock: return KEY_NUMLOCK;
        case ScrollLock:
        case Scroll: return KEY_SCROLLLOCK;
        
        // Numpad numbers
        case Numpad0: return KEY_KP0;
        case Numpad1: return KEY_KP1;
        case Numpad2: return KEY_KP2;
        case Numpad3: return KEY_KP3;
        case Numpad4: return KEY_KP4;
        case Numpad5: return KEY_KP5;
        case Numpad6: return KEY_KP6;
        case Numpad7: return KEY_KP7;
        case Numpad8: return KEY_KP8;
        case Numpad9: return KEY_KP9;
        
        // Numpad operators
        case Add: return KEY_KPPLUS;
        case Subtract: return KEY_KPMINUS;
        case Multiply: return KEY_KPASTERISK;
        case Divide: return KEY_KPSLASH;
        case Decimal: return KEY_KPDOT;
        case Separator: return KEY_KPCOMMA;
        
        // US keyboard symbols (OEM keys)
        case OEMMinus: return KEY_MINUS;        // - _
        case OEMPlus: return KEY_EQUAL;         // = +
        case OEM4: return KEY_LEFTBRACE;        // [ {
        case OEM6: return KEY_RIGHTBRACE;       // ] }
        case OEM5: return KEY_BACKSLASH;        // \ |
        case OEM1: return KEY_SEMICOLON;        // ; :
        case OEM7: return KEY_APOSTROPHE;       // ' "
        case OEMComma: return KEY_COMMA;        // , <
        case OEMPeriod: return KEY_DOT;         // . >
        case OEM2: return KEY_SLASH;            // / ?
        case OEM3: return KEY_GRAVE;            // ` ~
        
        // System keys
        case PrintScr: return KEY_SYSRQ;
        case Pause: return KEY_PAUSE;
        case Break: return KEY_BREAK;
        case Apps: return KEY_COMPOSE;          // Menu/Application key
        case Power: return KEY_POWER;
        case Sleep_k: return KEY_SLEEP;
        
        // Media keys
        case VolumeUp: return KEY_VOLUMEUP;
        case VolumeDown: return KEY_VOLUMEDOWN;
        case VolumeMute: return KEY_MUTE;
        case MicMute: return KEY_MICMUTE;
        case MediaPlayPause: return KEY_PLAYPAUSE;
        case MediaStop: return KEY_STOPCD;
        case MediaNextTrack: return KEY_NEXTSONG;
        case MediaPrevTrack: return KEY_PREVIOUSSONG;
        case MediaFast: return KEY_FASTFORWARD;
        case MediaRewind: return KEY_REWIND;
        
        // Browser keys
        case BrowserBack: return KEY_BACK;
        case BrowserForward: return KEY_FORWARD;
        case BrowserRefresh: return KEY_REFRESH;
        case BrowserStop: return KEY_STOP;
        case BrowserSearch: return KEY_SEARCH;
        case BrowserFavorites: return KEY_BOOKMARKS;
        case BrowserHome: return KEY_HOMEPAGE;
        
        // Application launch keys
        case LaunchMail: return KEY_MAIL;
        case LaunchMediaSelect: return KEY_MEDIA;
        case LaunchApp1: return KEY_PROG1;
        case LaunchApp2: return KEY_PROG2;
        
        // Brightness keys
        case BrightnessUp: return KEY_BRIGHTNESSUP;
        case BrightnessDown: return KEY_BRIGHTNESSDOWN;
        
        // Other special keys
        case Help: return KEY_HELP;
        case Undo: return KEY_UNDO;
        case Redo: return KEY_REDO;
        case Find: return KEY_FIND;
        case Select: return KEY_SELECT;
        case Execute: return KEY_OPEN;
        case Eject: return KEY_EJECTCD;
        case Zoom: return KEY_ZOOM;
        
        // Asian input method keys
        case Kana:
        case Hangeul:
        case Hangul: return KEY_HANGEUL;
        case Hanja: return KEY_HANJA;
        case Kanji: return KEY_KATAKANA;
        case Convert: return KEY_HENKAN;
        case NonConvert: return KEY_MUHENKAN;
        
        // Keys with no direct Linux equivalent or rarely used
        case None:
        case Cancel:
        case Clear:
        case Final:
        case Accept:
        case ModeChange:
        case Processkey:
        case Attn:
        case Crsel:
        case Exsel:
        case Ereof:
        case Play:
        case Packet:
        case NoName:
        case PA1:
        case OEMClear:
            // Return -1 for unsupported keys
            return -1;
            
        // Gamepad keys - not applicable for keyboard
        case GamepadA:
        case GamepadB:
        case GamepadX:
        case GamepadY:
        case GamepadDPadUp:
        case GamepadDPadDown:
        case GamepadDPadLeft:
        case GamepadDPadRight:
        case GamepadLeftShoulder:
        case GamepadRightShoulder:
        case GamepadLeftTrigger:
        case GamepadRightTrigger:
        case GamepadLeftThumbstickButton:
        case GamepadRightThumbstickButton:
        case GamepadLeftThumbstickUp:
        case GamepadLeftThumbstickDown:
        case GamepadLeftThumbstickLeft:
        case GamepadLeftThumbstickRight:
        case GamepadRightThumbstickUp:
        case GamepadRightThumbstickDown:
        case GamepadRightThumbstickLeft:
        case GamepadRightThumbstickRight:
        case GamepadMenu:
        case GamepadView:
            return -1;  // Gamepad not supported
            
        default:
            // Unsupported key
            return -1;
    }
}

int input_linux_init(void) {
    YA_LOG_INFO("Initializing Linux uinput backend");
    
    // Open /dev/uinput
    uinput_fd = open("/dev/uinput", O_WRONLY | O_NONBLOCK);
    if (uinput_fd < 0) {
        YA_LOG_ERROR("Failed to open /dev/uinput: %s (errno=%d)", strerror(errno), errno);
        if (errno == EACCES || errno == EPERM) {
            YA_LOG_WARN("Permission denied. Please configure udev rules for /dev/uinput");
        }
        return INPUT_BACKEND_ERROR_DEVICE;
    }

    // Enable event types
    if (ioctl(uinput_fd, UI_SET_EVBIT, EV_KEY) < 0 ||
        ioctl(uinput_fd, UI_SET_EVBIT, EV_REL) < 0 ||
        ioctl(uinput_fd, UI_SET_EVBIT, EV_SYN) < 0) {
        YA_LOG_ERROR("Failed to enable event types: %s", strerror(errno));
        close(uinput_fd);
        uinput_fd = -1;
        return INPUT_BACKEND_ERROR_INIT;
    }

    // Enable mouse buttons
    ioctl(uinput_fd, UI_SET_KEYBIT, BTN_LEFT);
    ioctl(uinput_fd, UI_SET_KEYBIT, BTN_RIGHT);
    ioctl(uinput_fd, UI_SET_KEYBIT, BTN_MIDDLE);

    // Enable relative axes for mouse movement and scrolling
    ioctl(uinput_fd, UI_SET_RELBIT, REL_X);
    ioctl(uinput_fd, UI_SET_RELBIT, REL_Y);
    ioctl(uinput_fd, UI_SET_RELBIT, REL_WHEEL);
    ioctl(uinput_fd, UI_SET_RELBIT, REL_HWHEEL);
    ioctl(uinput_fd, UI_SET_RELBIT, REL_WHEEL_HI_RES);
    ioctl(uinput_fd, UI_SET_RELBIT, REL_HWHEEL_HI_RES);

    const int enabled_keys[] = {
        // Digits (top row)
        KEY_0, KEY_1, KEY_2, KEY_3, KEY_4,
        KEY_5, KEY_6, KEY_7, KEY_8, KEY_9,

        // Letters
        KEY_A, KEY_B, KEY_C, KEY_D, KEY_E,
        KEY_F, KEY_G, KEY_H, KEY_I, KEY_J,
        KEY_K, KEY_L, KEY_M, KEY_N, KEY_O,
        KEY_P, KEY_Q, KEY_R, KEY_S, KEY_T,
        KEY_U, KEY_V, KEY_W, KEY_X, KEY_Y, KEY_Z,

        // OEM / punctuation keys for US layout
        KEY_MINUS, KEY_EQUAL, KEY_LEFTBRACE, KEY_RIGHTBRACE,
        KEY_BACKSLASH, KEY_SEMICOLON, KEY_APOSTROPHE,
        KEY_GRAVE, KEY_COMMA, KEY_DOT, KEY_SLASH,

        // Function keys
        KEY_ESC,
        KEY_F1, KEY_F2, KEY_F3, KEY_F4, KEY_F5, KEY_F6,
        KEY_F7, KEY_F8, KEY_F9, KEY_F10, KEY_F11, KEY_F12,
        KEY_F13, KEY_F14, KEY_F15, KEY_F16, KEY_F17, KEY_F18,
        KEY_F19, KEY_F20, KEY_F21, KEY_F22, KEY_F23, KEY_F24,

        // Modifiers
        KEY_LEFTSHIFT, KEY_RIGHTSHIFT,
        KEY_LEFTCTRL, KEY_RIGHTCTRL,
        KEY_LEFTALT, KEY_RIGHTALT,
        KEY_LEFTMETA, KEY_RIGHTMETA,
        KEY_CAPSLOCK, KEY_NUMLOCK, KEY_SCROLLLOCK,

        // Navigation / editing cluster
        KEY_INSERT, KEY_DELETE, KEY_HOME, KEY_END,
        KEY_PAGEUP, KEY_PAGEDOWN,
        KEY_UP, KEY_DOWN, KEY_LEFT, KEY_RIGHT,

        // Editing and whitespace
        KEY_TAB, KEY_ENTER, KEY_SPACE, KEY_BACKSPACE,

        // Ten-key / numeric keypad
        KEY_KP0, KEY_KP1, KEY_KP2, KEY_KP3, KEY_KP4,
        KEY_KP5, KEY_KP6, KEY_KP7, KEY_KP8, KEY_KP9,
        KEY_KPPLUS, KEY_KPMINUS, KEY_KPASTERISK, KEY_KPSLASH,
        KEY_KPDOT, KEY_KPENTER, KEY_KPCOMMA,
        KEY_KPLEFTPAREN, KEY_KPRIGHTPAREN,

        // System / application keys typically present on full keyboards
        KEY_PRINT, KEY_SYSRQ, KEY_SCROLLUP, KEY_SCROLLDOWN,
        KEY_PAUSE, KEY_BREAK, KEY_STOP, KEY_MAIL,
        KEY_HOMEPAGE, KEY_FORWARD, KEY_BACK,
        KEY_REFRESH, KEY_COMPUTER, KEY_CALC,
        KEY_SLEEP, KEY_WAKEUP,

        // Media keys (volume and playback)
        KEY_MUTE, KEY_VOLUMEDOWN, KEY_VOLUMEUP,
        KEY_NEXTSONG, KEY_PREVIOUSSONG,
        KEY_PLAYPAUSE, KEY_STOPCD, KEY_RECORD,

        // Brightness and display related
        KEY_BRIGHTNESSDOWN, KEY_BRIGHTNESSUP,
        KEY_DISPLAY_OFF,

        // Menu / application (context) key
        KEY_COMPOSE
    };

    for (size_t idx = 0; idx < sizeof(enabled_keys) / sizeof(enabled_keys[0]); ++idx) {
        ioctl(uinput_fd, UI_SET_KEYBIT, enabled_keys[idx]);
    }

    // Setup device
    struct uinput_setup usetup;
    memset(&usetup, 0, sizeof(usetup));
    usetup.id.bustype = BUS_USB;
    usetup.id.vendor = 0x8888;  // Dummy vendor ID
    usetup.id.product = 0x8888; // Dummy product ID
    snprintf(usetup.name, UINPUT_MAX_NAME_SIZE, "MouseHero Virtual Input");

    if (ioctl(uinput_fd, UI_DEV_SETUP, &usetup) < 0) {
        YA_LOG_ERROR("Failed to setup device: %s", strerror(errno));
        close(uinput_fd);
        uinput_fd = -1;
        return INPUT_BACKEND_ERROR_INIT;
    }

    // Create the device
    if (ioctl(uinput_fd, UI_DEV_CREATE) < 0) {
        YA_LOG_ERROR("Failed to create device: %s", strerror(errno));
        close(uinput_fd);
        uinput_fd = -1;
        return INPUT_BACKEND_ERROR_INIT;
    }

    YA_LOG_INFO("Linux uinput backend initialized successfully");
    return INPUT_BACKEND_OK;
}

void input_linux_shutdown(void) {
    if (uinput_fd >= 0) {
        close(uinput_fd);
        uinput_fd = -1;
    }
}

// Helper function to emit input event
static int emit_event(int type, int code, int value) {
    struct input_event ev;
    memset(&ev, 0, sizeof(ev));
    ev.type = type;
    ev.code = code;
    ev.value = value;
    
    if (write(uinput_fd, &ev, sizeof(ev)) < 0) {
        return -1;
    }
    return 0;
}

// Helper function to emit SYN event
static int emit_syn(void) {
    return emit_event(EV_SYN, SYN_REPORT, 0);
}

int input_linux_mouse_move(int32_t dx, int32_t dy) {
    if (uinput_fd < 0) {
        return INPUT_BACKEND_ERROR_INIT;
    }
    
    // Emit relative movement events
    if (dx != 0) {
        if (emit_event(EV_REL, REL_X, dx) < 0) {
            return INPUT_BACKEND_ERROR_DEVICE;
        }
    }
    
    if (dy != 0) {
        if (emit_event(EV_REL, REL_Y, dy) < 0) {
            return INPUT_BACKEND_ERROR_DEVICE;
        }
    }
    
    // Synchronize
    if (emit_syn() < 0) {
        return INPUT_BACKEND_ERROR_DEVICE;
    }
    
    return INPUT_BACKEND_OK;
}

int input_linux_mouse_press(mouse_button_t button) {
    if (uinput_fd < 0) {
        return INPUT_BACKEND_ERROR_INIT;
    }

    int btn_code = button_to_linux_code(button);
    if (btn_code < 0) {
        return INPUT_BACKEND_ERROR_INVALID_PARAM;
    }

    if (emit_event(EV_KEY, btn_code, 1) < 0) {
        return INPUT_BACKEND_ERROR_DEVICE;
    }
    if (emit_syn() < 0) {
        return INPUT_BACKEND_ERROR_DEVICE;
    }

    return INPUT_BACKEND_OK;
}

int input_linux_mouse_release(mouse_button_t button) {
    if (uinput_fd < 0) {
        return INPUT_BACKEND_ERROR_INIT;
    }

    int btn_code = button_to_linux_code(button);
    if (btn_code < 0) {
        return INPUT_BACKEND_ERROR_INVALID_PARAM;
    }

    if (emit_event(EV_KEY, btn_code, 0) < 0) {
        return INPUT_BACKEND_ERROR_DEVICE;
    }
    if (emit_syn() < 0) {
        return INPUT_BACKEND_ERROR_DEVICE;
    }

    return INPUT_BACKEND_OK;
}

int input_linux_mouse_click(mouse_button_t button) {
    int result = input_linux_mouse_press(button);
    if (result != INPUT_BACKEND_OK) {
        return result;
    }

    result = input_linux_mouse_release(button);
    if (result != INPUT_BACKEND_OK) {
        return result;
    }

    return INPUT_BACKEND_OK;
}

int input_linux_mouse_scroll(scroll_direction_t direction, int32_t amount) {
    if (uinput_fd < 0) {
        return INPUT_BACKEND_ERROR_INIT;
    }
    
    if (amount <= 0) {
        return INPUT_BACKEND_ERROR_INVALID_PARAM;
    }
    
    // Map direction to REL_WHEEL or REL_HWHEEL
    int event_type;
    int value;
    
    switch (direction) {
        case SCROLL_UP:
            event_type = REL_WHEEL;
            value = amount;  // Positive for up
            break;
        case SCROLL_DOWN:
            event_type = REL_WHEEL;
            value = -amount;  // Negative for down
            break;
        case SCROLL_LEFT:
            event_type = REL_HWHEEL;
            value = -amount;  // Negative for left
            break;
        case SCROLL_RIGHT:
            event_type = REL_HWHEEL;
            value = amount;  // Positive for right
            break;
        default:
            return INPUT_BACKEND_ERROR_INVALID_PARAM;
    }
    
    // Emit scroll event
    if (emit_event(EV_REL, event_type, value) < 0) {
        return INPUT_BACKEND_ERROR_DEVICE;
    }
    if (emit_syn() < 0) {
        return INPUT_BACKEND_ERROR_DEVICE;
    }
    
    return INPUT_BACKEND_OK;
}

int input_linux_key_press(uint32_t keycode) {
    if (uinput_fd < 0) {
        return INPUT_BACKEND_ERROR_INIT;
    }
    
    // keycode is expected to be a Linux KEY_* code
    // Press
    if (emit_event(EV_KEY, keycode, 1) < 0) {
        return INPUT_BACKEND_ERROR_DEVICE;
    }
    if (emit_syn() < 0) {
        return INPUT_BACKEND_ERROR_DEVICE;
    }
    
    // Release
    if (emit_event(EV_KEY, keycode, 0) < 0) {
        return INPUT_BACKEND_ERROR_DEVICE;
    }
    if (emit_syn() < 0) {
        return INPUT_BACKEND_ERROR_DEVICE;
    }
    
    return INPUT_BACKEND_OK;
}

int input_linux_diagnose(input_linux_diagnose_t *result) {
    if (!result) {
        return INPUT_BACKEND_ERROR_INVALID_PARAM;
    }
    
    memset(result, 0, sizeof(input_linux_diagnose_t));
    
    // Check if /dev/uinput exists
    result->device_exists = (access("/dev/uinput", F_OK) == 0);
    
    // Check write permission
    result->has_write_permission = (access("/dev/uinput", W_OK) == 0);
    
    // TODO: Check udev rule detection (scan /etc/udev/rules.d/ for uinput rules)
    result->udev_rule_detected = false;
    
    // Verify non-root (EUID != 0)
    uid_t euid = geteuid();
    result->non_root_verified = (euid != 0);
    
    // Build error message with actionable guidance
    if (!result->device_exists) {
        snprintf(result->error_message, sizeof(result->error_message),
                 "/dev/uinput does not exist. Load kernel module: sudo modprobe uinput");
    } else if (euid == 0) {
        snprintf(result->error_message, sizeof(result->error_message),
                 "Running as root (EUID=%u). Application should run as non-root user with udev rules.", euid);
    } else if (!result->has_write_permission) {
        // Non-root but no write permission - need udev rules
        snprintf(result->error_message, sizeof(result->error_message),
                 "No write permission to /dev/uinput (EUID=%u). Configure udev rules and reload: "
                 "sudo udevadm control --reload-rules && sudo udevadm trigger", euid);
    } else {
        // All checks passed
        snprintf(result->error_message, sizeof(result->error_message), 
                 "OK: Non-root user (EUID=%u) can write to /dev/uinput", euid);
    }
    
    return INPUT_BACKEND_OK;
}

// High-level key action wrapper for key_inject.c integration
int input_linux_key_action(enum CKey key, enum CDirection direction) {
    if (uinput_fd < 0) {
        YA_LOG_ERROR("Input backend not initialized");
        return INPUT_BACKEND_ERROR_INIT;
    }
    
    // Map CKey to Linux keycode
    int linux_keycode = ckey_to_linux_keycode(key);
    if (linux_keycode < 0) {
        // Unsupported key
        YA_LOG_WARN("Unsupported CKey: %d", key);
        return INPUT_BACKEND_ERROR_INVALID_PARAM;
    }
    
    // Handle different directions
    if (direction == Press) {
        // Press only
        if (emit_event(EV_KEY, linux_keycode, 1) < 0) {
            return INPUT_BACKEND_ERROR_DEVICE;
        }
        if (emit_syn() < 0) {
            return INPUT_BACKEND_ERROR_DEVICE;
        }
    } else if (direction == Release) {
        // Release only
        if (emit_event(EV_KEY, linux_keycode, 0) < 0) {
            return INPUT_BACKEND_ERROR_DEVICE;
        }
        if (emit_syn() < 0) {
            return INPUT_BACKEND_ERROR_DEVICE;
        }
    } else if (direction == Click) {
        // Press and release
        if (emit_event(EV_KEY, linux_keycode, 1) < 0) {
            return INPUT_BACKEND_ERROR_DEVICE;
        }
        if (emit_syn() < 0) {
            return INPUT_BACKEND_ERROR_DEVICE;
        }
        if (emit_event(EV_KEY, linux_keycode, 0) < 0) {
            return INPUT_BACKEND_ERROR_DEVICE;
        }
        if (emit_syn() < 0) {
            return INPUT_BACKEND_ERROR_DEVICE;
        }
    }
    
    return INPUT_BACKEND_OK;
}

// Raw evdev keycode injection (for xkb mapping)
int input_linux_key_action_raw(int evdev_key, enum CDirection direction) {
    if (uinput_fd < 0) {
        YA_LOG_ERROR("Input backend not initialized");
        return INPUT_BACKEND_ERROR_INIT;
    }
    
    // Handle different directions
    if (direction == Press) {
        if (emit_event(EV_KEY, evdev_key, 1) < 0) {
            return INPUT_BACKEND_ERROR_DEVICE;
        }
        if (emit_syn() < 0) {
            return INPUT_BACKEND_ERROR_DEVICE;
        }
    } else if (direction == Release) {
        if (emit_event(EV_KEY, evdev_key, 0) < 0) {
            return INPUT_BACKEND_ERROR_DEVICE;
        }
        if (emit_syn() < 0) {
            return INPUT_BACKEND_ERROR_DEVICE;
        }
    } else if (direction == Click) {
        if (emit_event(EV_KEY, evdev_key, 1) < 0) {
            return INPUT_BACKEND_ERROR_DEVICE;
        }
        if (emit_syn() < 0) {
            return INPUT_BACKEND_ERROR_DEVICE;
        }
        if (emit_event(EV_KEY, evdev_key, 0) < 0) {
            return INPUT_BACKEND_ERROR_DEVICE;
        }
        if (emit_syn() < 0) {
            return INPUT_BACKEND_ERROR_DEVICE;
        }
    }
    
    return INPUT_BACKEND_OK;
}

#endif // __linux__
