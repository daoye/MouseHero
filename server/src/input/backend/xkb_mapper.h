#ifndef INPUT_BACKEND_XKB_MAPPER_H
#define INPUT_BACKEND_XKB_MAPPER_H

#include <stdint.h>
#include <stdbool.h>

#ifdef __cplusplus
extern "C" {
#endif

/**
 * XKB Mapping Module for Linux
 * 
 * Provides character → keycode + modifiers mapping using libxkbcommon.
 * Auto-detects keyboard layout from X11 or environment variables.
 * 
 * Layout detection priority:
 * 1. X11 (if DISPLAY available and libxkbcommon-x11 linked)
 * 2. Environment variables (XKB_DEFAULT_LAYOUT, XKB_DEFAULT_VARIANT, XKB_DEFAULT_OPTIONS)
 * 3. Fallback to "us"
 */

/**
 * Initialize XKB mapping system
 * Auto-detects layout and builds reverse mapping cache
 * @return 0 on success, -1 on failure
 */
int xkbmap_init_auto(void);

/**
 * Shutdown and free XKB mapping resources
 */
void xkbmap_free(void);

/**
 * Map a UTF-8 character to evdev keycode and modifiers
 * @param utf8 UTF-8 encoded character (single code point)
 * @param evdev_key Output: Linux evdev KEY_* code (e.g., KEY_A)
 * @param mods_mask Output: Required modifiers bitmask (bit 0=Shift, bit 1=AltGr/Level3, bit 2=Ctrl, bit 3=Meta)
 * @return true if mapped successfully, false if unmappable (dead key/compose/not found)
 */
bool xkbmap_map_utf8_to_evdev(const char* utf8, int* evdev_key, unsigned* mods_mask);

/**
 * Get layout source description for logging
 * @return String like "X11", "env:us", "fallback:us", or "uninitialized"
 */
const char* xkbmap_get_layout_source(void);

#ifdef __cplusplus
}
#endif

#endif // INPUT_BACKEND_XKB_MAPPER_H
