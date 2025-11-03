#include "input/keyboard/clipboard.h"
#include "input/facade.h"
#include "ya_logger.h"
#include "ya_config.h"
#include <string.h>

// External config
extern YA_Config config;

// Global configuration
static clipboard_fallback_config_t g_clipboard_config = {
    .enabled = true,
    .use_ctrl_v = false,  // Default: Shift+Insert
};

// Timing constants (from ya_server_handler.c)
#define YA_KEY_DELAY_MS 10

// Forward declarations for lock/sleep (from ya_server_handler.c)
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

void clipboard_helper_init(void) {
    // Read [input].clipboard_fallback
    const char* fallback_str = ya_config_get(&config, "input", "clipboard_fallback");
    if (fallback_str) {
        g_clipboard_config.enabled = (strcmp(fallback_str, "1") == 0 || 
                                       strcmp(fallback_str, "true") == 0 || 
                                       strcmp(fallback_str, "on") == 0 || 
                                       strcmp(fallback_str, "yes") == 0);
    }
    
    // Read [input].clipboard_paste_key
    const char* paste_key = ya_config_get(&config, "input", "clipboard_paste_key");
    if (paste_key) {
        if (strcmp(paste_key, "ctrl_v") == 0) {
            g_clipboard_config.use_ctrl_v = true;
        } else {
            g_clipboard_config.use_ctrl_v = false;  // Default: shift_insert
        }
    }

    YA_LOG_INFO("Clipboard helper initialized: enabled=%d, paste_key=%s",
                g_clipboard_config.enabled,
                g_clipboard_config.use_ctrl_v ? "Ctrl+V" : "Shift+Insert");
}

const clipboard_fallback_config_t* clipboard_helper_get_config(void) {
    return &g_clipboard_config;
}

YAError clipboard_paste_text(const char* text) {
    if (!text || text[0] == '\0') {
        return InvalidInput;
    }
    
    if (!g_clipboard_config.enabled) {
        YA_LOG_DEBUG("Clipboard fallback disabled by config");
        return UnsupportedOperation;
    }

    // Backup original clipboard
    const char* backup = clipboard_get();
    
    // Set clipboard to target text
    YAError set_err = clipboard_set(text);
    if (set_err != Success) {
        if (backup) {
            free_string((char*)backup);
        }
        return set_err;
    }
    
    // Send paste shortcut (with lock and timing)
    ya_key_lock();

    YAError key_err = Success;
    if (g_clipboard_config.use_ctrl_v) {
        YA_LOG_DEBUG("clipboard fallback: sending Ctrl+V");
        key_err = input_key_action(Control, Press);
        if (key_err == Success) {
            ya_key_sleep_ms(YA_KEY_DELAY_MS);
            key_err = input_key_action(V, Click);
        }
        if (key_err == Success) {
            ya_key_sleep_ms(YA_KEY_DELAY_MS);
            key_err = input_key_action(Control, Release);
        }
    } else {
        YA_LOG_DEBUG("clipboard fallback: sending Shift+Insert");
        key_err = input_key_action(Shift, Press);
        if (key_err == Success) {
            ya_key_sleep_ms(YA_KEY_DELAY_MS);
            key_err = input_key_action(Insert, Click);
        }
        if (key_err == Success) {
            ya_key_sleep_ms(YA_KEY_DELAY_MS);
            key_err = input_key_action(Shift, Release);
        }
    }

    ya_key_unlock();

    if (key_err == Success) {
        ya_key_sleep_ms(YA_KEY_DELAY_MS);
    }

    // Restore original clipboard if configured
    if (backup) {
        clipboard_set(backup);
        free_string((char*)backup);
    }

    return key_err;
}
