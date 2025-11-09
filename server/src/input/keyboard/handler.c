// Keyboard input handler - unified character and function key processing
#include "input/keyboard/handler.h"
#include "input/facade.h"
#include "input/keyboard/clipboard.h"
#include "ya_logger.h"
#include "ya_event.h"
#include <string.h>

#ifdef USE_UINPUT
#include "input/backend/linux_uinput.h"
#include <linux/input-event-codes.h>

// Forward declarations from key_inject_new.h
YAError map_char_to_evdev_linux(int32_t codepoint, int* evdev_key, unsigned* mods_mask);
void encode_codepoint_to_utf8(int32_t codepoint, char* utf8_out);

#else
// Forward declarations from key_inject_new.h
YAError map_char_to_platform(int32_t codepoint, uint32_t* platform_code, bool* need_shift);
#endif

// Timing and lock (from ya_server_handler.c)
#define YA_KEY_DELAY_MS 10

#if defined(_WIN32)
#include <windows.h>
static CRITICAL_SECTION g_key_inject_cs;
static INIT_ONCE g_key_inject_once = INIT_ONCE_STATIC_INIT;
static BOOL CALLBACK ya_init_key_cs(PINIT_ONCE InitOnce, PVOID Parameter, PVOID *Context) {
    (void)InitOnce; (void)Parameter; (void)Context;
    InitializeCriticalSection(&g_key_inject_cs);
    return TRUE;
}
static inline void ya_key_lock(void) {
    InitOnceExecuteOnce(&g_key_inject_once, ya_init_key_cs, NULL, NULL);
    EnterCriticalSection(&g_key_inject_cs);
}
static inline void ya_key_unlock(void) {
    LeaveCriticalSection(&g_key_inject_cs);
}
static inline void ya_key_sleep_ms(int ms) {
    Sleep(ms);
}
#else
#include <pthread.h>
#include <unistd.h>
static pthread_mutex_t g_key_inject_mutex = PTHREAD_MUTEX_INITIALIZER;
static inline void ya_key_lock(void) {
    (void)pthread_mutex_lock(&g_key_inject_mutex);
}
static inline void ya_key_unlock(void) {
    (void)pthread_mutex_unlock(&g_key_inject_mutex);
}
static inline void ya_key_sleep_ms(int ms) {
    usleep(ms * 1000);
}
#endif

// ============================================================================
// Modifier Management
// ============================================================================

// Merge user-requested modifiers with xkb-required modifiers (dedup)
static unsigned merge_modifiers(uint32_t user_mods, unsigned xkb_mods) {
    unsigned final_mods = 0;
    
    // Shift: bit 0
    if ((user_mods & CHORD_MOD_SHIFT) || (xkb_mods & (1 << 0))) {
        final_mods |= (1 << 0);
    }
    
    // Level3/AltGr: bit 1 (only from xkb, user doesn't request this directly)
    if (xkb_mods & (1 << 1)) {
        final_mods |= (1 << 1);
    }
    
    // Ctrl: bit 2
    if ((user_mods & CHORD_MOD_CTRL) || (xkb_mods & (1 << 2))) {
        final_mods |= (1 << 2);
    }
    
    // Meta: bit 3
    if ((user_mods & CHORD_MOD_META) || (xkb_mods & (1 << 3))) {
        final_mods |= (1 << 3);
    }
    
    // Alt: user-requested only (xkb doesn't use Alt directly)
    // We'll handle this separately
    
    return final_mods;
}

#ifdef USE_UINPUT

// Press modifiers with timing
static YAError press_modifiers_unified(unsigned mods_mask, bool user_alt, bool* pressed_state) {
    // pressed_state[0]=Shift, [1]=AltGr, [2]=Ctrl, [3]=Meta, [4]=Alt
    memset(pressed_state, 0, 5 * sizeof(bool));
    
    if (mods_mask & (1 << 0)) { // Shift
        YA_LOG_DEBUG("Pressing Shift modifier");
        YAError err = input_key_action(Shift, Press);
        if (err != Success) {
            YA_LOG_ERROR("Failed to press Shift: error %d", err);
            return err;
        }
        pressed_state[0] = true;
        ya_key_sleep_ms(YA_KEY_DELAY_MS);
    }
    
    if (mods_mask & (1 << 1)) { // AltGr → RightAlt
        int result = input_linux_key_action_raw(KEY_RIGHTALT, Press);
        if (result != INPUT_BACKEND_OK) {
            if (pressed_state[0]) input_key_action(Shift, Release);
            return PlatformError;
        }
        pressed_state[1] = true;
        ya_key_sleep_ms(YA_KEY_DELAY_MS);
    }
    
    if (mods_mask & (1 << 2)) { // Ctrl
        YAError err = input_key_action(Control, Press);
        if (err != Success) {
            if (pressed_state[1]) input_linux_key_action_raw(KEY_RIGHTALT, Release);
            if (pressed_state[0]) input_key_action(Shift, Release);
            return err;
        }
        pressed_state[2] = true;
        ya_key_sleep_ms(YA_KEY_DELAY_MS);
    }
    
    if (mods_mask & (1 << 3)) { // Meta
        YAError err = input_key_action(Meta, Press);
        if (err != Success) {
            if (pressed_state[2]) input_key_action(Control, Release);
            if (pressed_state[1]) input_linux_key_action_raw(KEY_RIGHTALT, Release);
            if (pressed_state[0]) input_key_action(Shift, Release);
            return err;
        }
        pressed_state[3] = true;
        ya_key_sleep_ms(YA_KEY_DELAY_MS);
    }
    
    if (user_alt) { // User-requested Alt
        YAError err = input_key_action(Alt, Press);
        if (err != Success) {
            if (pressed_state[3]) input_key_action(Meta, Release);
            if (pressed_state[2]) input_key_action(Control, Release);
            if (pressed_state[1]) input_linux_key_action_raw(KEY_RIGHTALT, Release);
            if (pressed_state[0]) input_key_action(Shift, Release);
            return err;
        }
        pressed_state[4] = true;
        ya_key_sleep_ms(YA_KEY_DELAY_MS);
    }
    
    return Success;
}

// Release modifiers in reverse order
static void release_modifiers_unified(const bool* pressed_state) {
    if (pressed_state[4]) { // Alt
        input_key_action(Alt, Release);
        ya_key_sleep_ms(YA_KEY_DELAY_MS);
    }
    if (pressed_state[3]) { // Meta
        input_key_action(Meta, Release);
        ya_key_sleep_ms(YA_KEY_DELAY_MS);
    }
    if (pressed_state[2]) { // Ctrl
        input_key_action(Control, Release);
        ya_key_sleep_ms(YA_KEY_DELAY_MS);
    }
    if (pressed_state[1]) { // AltGr
        input_linux_key_action_raw(KEY_RIGHTALT, Release);
        ya_key_sleep_ms(YA_KEY_DELAY_MS);
    }
    if (pressed_state[0]) { // Shift
        input_key_action(Shift, Release);
        ya_key_sleep_ms(YA_KEY_DELAY_MS);
    }
}

// Linux character injection with unified modifier handling
static YAError input_char_linux_unified(int32_t codepoint, uint32_t user_mods, enum CDirection dir) {
    int evdev_key = 0;
    unsigned xkb_mods = 0;
    
    // Try xkb mapping
    YAError map_result = map_char_to_evdev_linux(codepoint, &evdev_key, &xkb_mods);
    
    if (map_result == Success) {
        // Merge user and xkb modifiers
        unsigned final_mods = merge_modifiers(user_mods, xkb_mods);
        bool user_alt = (user_mods & CHORD_MOD_ALT) != 0;
        
        YA_LOG_DEBUG("Injecting U+%04X: user_mods=0x%X, xkb_mods=0x%X, final_mods=0x%X, evdev=%d",
                     codepoint, user_mods, xkb_mods, final_mods, evdev_key);
        
        // Press modifiers
        bool pressed_state[5];
        YAError err = press_modifiers_unified(final_mods, user_alt, pressed_state);
        if (err != Success) {
            // Fallback to clipboard if modifier press fails
            char utf8[5] = {0};
            encode_codepoint_to_utf8(codepoint, utf8);
            return clipboard_paste_text(utf8);
        }
        
        // Inject main key
        // When modifiers are present and direction is Click, split into Press+Release
        // to ensure proper modifier registration
        YA_LOG_DEBUG("Injecting main key: evdev=%d, dir=%d", evdev_key, dir);
        int result;
        if (dir == Click && final_mods != 0) {
            // Press main key
            result = input_linux_key_action_raw(evdev_key, Press);
            if (result == INPUT_BACKEND_OK) {
                ya_key_sleep_ms(YA_KEY_DELAY_MS);
                // Release main key
                result = input_linux_key_action_raw(evdev_key, Release);
            }
        } else {
            // Normal injection without modifiers
            result = input_linux_key_action_raw(evdev_key, dir);
        }
        YA_LOG_DEBUG("Main key injection result: %d", result);
        
        // Release modifiers
        release_modifiers_unified(pressed_state);
        
        if (result != INPUT_BACKEND_OK) {
            YA_LOG_ERROR("xkb inject failed for U+%04X (evdev %d): backend error %d", codepoint, evdev_key, result);
            // Fallback to clipboard
            char utf8[5] = {0};
            encode_codepoint_to_utf8(codepoint, utf8);
            return clipboard_paste_text(utf8);
        }
        
        return Success;
    }
    
    // xkb mapping miss → clipboard fallback
    char utf8[5] = {0};
    encode_codepoint_to_utf8(codepoint, utf8);
    return clipboard_paste_text(utf8);
}

// Linux function key with modifiers
static YAError input_function_key_linux(enum CKey key, uint32_t user_mods, enum CDirection dir) {
    bool user_alt = (user_mods & CHORD_MOD_ALT) != 0;
    unsigned mods_mask = 0;
    
    if (user_mods & CHORD_MOD_SHIFT) mods_mask |= (1 << 0);
    if (user_mods & CHORD_MOD_CTRL) mods_mask |= (1 << 2);
    if (user_mods & CHORD_MOD_META) mods_mask |= (1 << 3);
    
    bool pressed_state[5];
    YAError err = press_modifiers_unified(mods_mask, user_alt, pressed_state);
    if (err != Success) return err;
    
    // Inject function key
    err = input_key_action(key, dir);
    
    // Release modifiers
    release_modifiers_unified(pressed_state);
    
    return err;
}

#else // Non-Linux

// Non-Linux character injection
static YAError input_char_unified(int32_t codepoint, uint32_t user_mods, enum CDirection dir) {
    uint32_t platform_code = 0;
    bool platform_shift = false;
    
    // Try platform mapping
    if (map_char_to_platform(codepoint, &platform_code, &platform_shift) == Success) {
        
        // Press user modifiers
        if (user_mods & CHORD_MOD_SHIFT) {
            input_key_action(Shift, Press);
            ya_key_sleep_ms(YA_KEY_DELAY_MS);
        }
        if (user_mods & CHORD_MOD_CTRL) {
            input_key_action(Control, Press);
            ya_key_sleep_ms(YA_KEY_DELAY_MS);
        }
        if (user_mods & CHORD_MOD_ALT) {
            input_key_action(Alt, Press);
            ya_key_sleep_ms(YA_KEY_DELAY_MS);
        }
        if (user_mods & CHORD_MOD_META) {
            input_key_action(Meta, Press);
            ya_key_sleep_ms(YA_KEY_DELAY_MS);
        }
        
        // Press platform shift if needed (and not already pressed)
        if (platform_shift && !(user_mods & CHORD_MOD_SHIFT)) {
            input_key_action(Shift, Press);
            ya_key_sleep_ms(YA_KEY_DELAY_MS);
        }
        
        // Inject key
        YAError err = key_action_with_platform_code(platform_code, dir);
        ya_key_sleep_ms(YA_KEY_DELAY_MS);
        
        // Release modifiers in reverse
        if (platform_shift && !(user_mods & CHORD_MOD_SHIFT)) {
            input_key_action(Shift, Release);
            ya_key_sleep_ms(YA_KEY_DELAY_MS);
        }
        if (user_mods & CHORD_MOD_META) {
            input_key_action(Meta, Release);
            ya_key_sleep_ms(YA_KEY_DELAY_MS);
        }
        if (user_mods & CHORD_MOD_ALT) {
            input_key_action(Alt, Release);
            ya_key_sleep_ms(YA_KEY_DELAY_MS);
        }
        if (user_mods & CHORD_MOD_CTRL) {
            input_key_action(Control, Release);
            ya_key_sleep_ms(YA_KEY_DELAY_MS);
        }
        if (user_mods & CHORD_MOD_SHIFT) {
            input_key_action(Shift, Release);
            ya_key_sleep_ms(YA_KEY_DELAY_MS);
        }
        
        return err;
    }
    
    // Fallback to unicode
    return key_unicode_action(codepoint, dir);
}

// Non-Linux function key
static YAError input_function_key(enum CKey key, uint32_t user_mods, enum CDirection dir) {
    if (user_mods & CHORD_MOD_SHIFT) {
        input_key_action(Shift, Press);
        ya_key_sleep_ms(YA_KEY_DELAY_MS);
    }
    if (user_mods & CHORD_MOD_CTRL) {
        input_key_action(Control, Press);
        ya_key_sleep_ms(YA_KEY_DELAY_MS);
    }
    if (user_mods & CHORD_MOD_ALT) {
        input_key_action(Alt, Press);
        ya_key_sleep_ms(YA_KEY_DELAY_MS);
    }
    if (user_mods & CHORD_MOD_META) {
        input_key_action(Meta, Press);
        ya_key_sleep_ms(YA_KEY_DELAY_MS);
    }
    
    YAError err = input_key_action(key, dir);
    ya_key_sleep_ms(YA_KEY_DELAY_MS);
    
    if (user_mods & CHORD_MOD_META) {
        input_key_action(Meta, Release);
        ya_key_sleep_ms(YA_KEY_DELAY_MS);
    }
    if (user_mods & CHORD_MOD_ALT) {
        input_key_action(Alt, Release);
        ya_key_sleep_ms(YA_KEY_DELAY_MS);
    }
    if (user_mods & CHORD_MOD_CTRL) {
        input_key_action(Control, Release);
        ya_key_sleep_ms(YA_KEY_DELAY_MS);
    }
    if (user_mods & CHORD_MOD_SHIFT) {
        input_key_action(Shift, Release);
        ya_key_sleep_ms(YA_KEY_DELAY_MS);
    }
    
    return err;
}

#endif

// ============================================================================
// Public API
// ============================================================================

YAError keyboard_char(int32_t codepoint, uint32_t mods, enum CDirection dir) {
    ya_key_lock();
    
#ifdef USE_UINPUT
    YAError err = input_char_linux_unified(codepoint, mods, dir);
#else
    YAError err = input_char_unified(codepoint, mods, dir);
#endif
    
    ya_key_unlock();
    return err;
}

YAError keyboard_function_key(enum CKey key, uint32_t mods, enum CDirection dir) {
    ya_key_lock();
    
#ifdef USE_UINPUT
    YAError err = input_function_key_linux(key, mods, dir);
#else
    YAError err = input_function_key(key, mods, dir);
#endif
    
    ya_key_unlock();
    return err;
}
