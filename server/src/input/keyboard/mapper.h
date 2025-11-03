#pragma once
#include <stdint.h>
#include <stdbool.h>
#include "rs.h"

#ifdef __cplusplus
extern "C" {
#endif

// ============================================================================
// Refactored key_inject API - Pure mapping functions
// ============================================================================

#ifdef USE_UINPUT
/**
 * Map character codepoint to Linux evdev keycode + modifiers via xkb
 * @param codepoint Unicode codepoint
 * @param evdev_key Output: Linux KEY_* code
 * @param mods_mask Output: Modifier bitmask (bit0=Shift, bit1=Level3, bit2=Ctrl, bit3=Meta)
 * @return Success if mapped, NotFound if unmappable, InvalidInput if invalid codepoint
 */
YAError map_char_to_evdev_linux(int32_t codepoint, int* evdev_key, unsigned* mods_mask);

/**
 * Encode codepoint to UTF-8 string (for clipboard fallback)
 * @param codepoint Unicode codepoint
 * @param utf8_out Output buffer (must be at least 5 bytes)
 */
void encode_codepoint_to_utf8(int32_t codepoint, char* utf8_out);

#else
/**
 * Map character to platform-specific keycode (macOS/Windows)
 * @param codepoint Unicode codepoint
 * @param platform_code Output: Platform keycode (CGKeyCode or VK_*)
 * @param need_shift Output: Whether Shift modifier is needed
 * @return Success if mapped, NotFound if unmappable
 */
YAError map_char_to_platform(int32_t codepoint, uint32_t* platform_code, bool* need_shift);
#endif

#ifdef __cplusplus
}
#endif
