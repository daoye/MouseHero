#ifndef CLIPBOARD_HELPER_H
#define CLIPBOARD_HELPER_H

#include "rs.h"
#include <stdbool.h>

#ifdef __cplusplus
extern "C" {
#endif

/**
 * Clipboard fallback configuration
 */
typedef struct {
    bool enabled;           // Whether clipboard fallback is enabled
    bool use_ctrl_v;        // true=Ctrl+V, false=Shift+Insert
} clipboard_fallback_config_t;

/**
 * Initialize clipboard helper with configuration from ya_config
 * Reads [input] section: clipboard_fallback, clipboard_paste_key, clipboard_restore
 */
void clipboard_helper_init(void);

/**
 * Get current clipboard fallback configuration
 */
const clipboard_fallback_config_t* clipboard_helper_get_config(void);

/**
 * Paste text via clipboard (with backup/restore if configured)
 * @param text UTF-8 text to paste
 * @return Success or error code
 */
YAError clipboard_paste_text(const char* text);

#ifdef __cplusplus
}
#endif

#endif // CLIPBOARD_HELPER_H
