// Refactored key_inject.c - Pure character mapping, no modifier/clipboard logic
#include "input/keyboard/mapper.h"
#include "rs.h"
#include "ya_logger.h"
#include <stdint.h>
#include <string.h>

#ifdef USE_UINPUT
#include "input/backend/xkb_mapper.h"

// ============================================================================
// Linux: Character → evdev mapping via xkb
// ============================================================================

YAError map_char_to_evdev_linux(int32_t codepoint, int* evdev_key, unsigned* mods_mask) {
    // UTF-8 encode
    char utf8[5] = {0};
    if (codepoint > 0 && codepoint < 0x80) {
        utf8[0] = (char)codepoint;
    } else if (codepoint < 0x800) {
        utf8[0] = (char)(0xC0 | (codepoint >> 6));
        utf8[1] = (char)(0x80 | (codepoint & 0x3F));
    } else if (codepoint < 0x10000) {
        utf8[0] = (char)(0xE0 | (codepoint >> 12));
        utf8[1] = (char)(0x80 | ((codepoint >> 6) & 0x3F));
        utf8[2] = (char)(0x80 | (codepoint & 0x3F));
    } else if (codepoint < 0x110000) {
        utf8[0] = (char)(0xF0 | (codepoint >> 18));
        utf8[1] = (char)(0x80 | ((codepoint >> 12) & 0x3F));
        utf8[2] = (char)(0x80 | ((codepoint >> 6) & 0x3F));
        utf8[3] = (char)(0x80 | (codepoint & 0x3F));
    }
    
    if (utf8[0] == '\0') {
        YA_LOG_ERROR("Invalid codepoint for UTF-8 encoding: %d", codepoint);
        return InvalidInput;
    }
    
    // Try xkb mapping
    if (xkbmap_map_utf8_to_evdev(utf8, evdev_key, mods_mask)) {
        YA_LOG_DEBUG("xkb mapped U+%04X ('%c') → evdev=%d, mods=0x%X", 
                     codepoint, (codepoint >= 0x20 && codepoint < 0x7F) ? (char)codepoint : '?',
                     *evdev_key, *mods_mask);
        return Success;
    }
    
    // Not found - caller should decide fallback strategy
    YA_LOG_TRACE("xkb mapping miss for U+%04X", codepoint);
    return NotFound;
}

// Helper: encode codepoint to UTF-8 (for clipboard fallback)
void encode_codepoint_to_utf8(int32_t codepoint, char* utf8_out) {
    if (!utf8_out) return;
    
    if (codepoint > 0 && codepoint < 0x80) {
        utf8_out[0] = (char)codepoint;
    } else if (codepoint < 0x800) {
        utf8_out[0] = (char)(0xC0 | (codepoint >> 6));
        utf8_out[1] = (char)(0x80 | (codepoint & 0x3F));
    } else if (codepoint < 0x10000) {
        utf8_out[0] = (char)(0xE0 | (codepoint >> 12));
        utf8_out[1] = (char)(0x80 | ((codepoint >> 6) & 0x3F));
        utf8_out[2] = (char)(0x80 | (codepoint & 0x3F));
    } else if (codepoint < 0x110000) {
        utf8_out[0] = (char)(0xF0 | (codepoint >> 18));
        utf8_out[1] = (char)(0x80 | ((codepoint >> 12) & 0x3F));
        utf8_out[2] = (char)(0x80 | ((codepoint >> 6) & 0x3F));
        utf8_out[3] = (char)(0x80 | (codepoint & 0x3F));
    }
}

#else // Non-Linux platforms

// ============================================================================
// macOS: US layout ASCII → CGKeyCode mapping
// ============================================================================

#if defined(__APPLE__)
bool map_char_to_platform_macos(int32_t ch, uint32_t *platform_code, bool *need_shift) {
    if (!platform_code || !need_shift) return false;
    *need_shift = false;
    
    // clang-format off
    switch (ch) {
    case '1': *platform_code = 0x12; return true; case '!': *need_shift = true; *platform_code = 0x12; return true;
    case '2': *platform_code = 0x13; return true; case '@': *need_shift = true; *platform_code = 0x13; return true;
    case '3': *platform_code = 0x14; return true; case '#': *need_shift = true; *platform_code = 0x14; return true;
    case '4': *platform_code = 0x15; return true; case '$': *need_shift = true; *platform_code = 0x15; return true;
    case '5': *platform_code = 0x17; return true; case '%': *need_shift = true; *platform_code = 0x17; return true;
    case '6': *platform_code = 0x16; return true; case '^': *need_shift = true; *platform_code = 0x16; return true;
    case '7': *platform_code = 0x1A; return true; case '&': *need_shift = true; *platform_code = 0x1A; return true;
    case '8': *platform_code = 0x1C; return true; case '*': *need_shift = true; *platform_code = 0x1C; return true;
    case '9': *platform_code = 0x19; return true; case '(': *need_shift = true; *platform_code = 0x19; return true;
    case '0': *platform_code = 0x1D; return true; case ')': *need_shift = true; *platform_code = 0x1D; return true;
    case '-': *platform_code = 0x1B; return true; case '_': *need_shift = true; *platform_code = 0x1B; return true;
    case '=': *platform_code = 0x18; return true; case '+': *need_shift = true; *platform_code = 0x18; return true;
    case '`': *platform_code = 0x32; return true; case '~': *need_shift = true; *platform_code = 0x32; return true;
    case '[': *platform_code = 0x21; return true; case '{': *need_shift = true; *platform_code = 0x21; return true;
    case ']': *platform_code = 0x1E; return true; case '}': *need_shift = true; *platform_code = 0x1E; return true;
    case '\\': *platform_code = 0x2A; return true; case '|': *need_shift = true; *platform_code = 0x2A; return true;
    case ';': *platform_code = 0x29; return true; case ':': *need_shift = true; *platform_code = 0x29; return true;
    case '\'': *platform_code = 0x27; return true; case '"': *need_shift = true; *platform_code = 0x27; return true;
    case ',': *platform_code = 0x2B; return true; case '<': *need_shift = true; *platform_code = 0x2B; return true;
    case '.': *platform_code = 0x2F; return true; case '>': *need_shift = true; *platform_code = 0x2F; return true;
    case '/': *platform_code = 0x2C; return true; case '?': *need_shift = true; *platform_code = 0x2C; return true;
    default: return false;
    }
    // clang-format on
}
#endif

// ============================================================================
// Windows: US layout ASCII → Virtual-Key mapping
// ============================================================================

#if defined(_WIN32)
bool map_char_to_platform_windows(int32_t ch, uint32_t *vk_code, bool *need_shift) {
    if (!vk_code || !need_shift) return false;
    *need_shift = false;
    
    // clang-format off
    switch (ch) {
    case '0': *vk_code = 0x30; return true; case ')': *need_shift = true; *vk_code = 0x30; return true;
    case '1': *vk_code = 0x31; return true; case '!': *need_shift = true; *vk_code = 0x31; return true;
    case '2': *vk_code = 0x32; return true; case '@': *need_shift = true; *vk_code = 0x32; return true;
    case '3': *vk_code = 0x33; return true; case '#': *need_shift = true; *vk_code = 0x33; return true;
    case '4': *vk_code = 0x34; return true; case '$': *need_shift = true; *vk_code = 0x34; return true;
    case '5': *vk_code = 0x35; return true; case '%': *need_shift = true; *vk_code = 0x35; return true;
    case '6': *vk_code = 0x36; return true; case '^': *need_shift = true; *vk_code = 0x36; return true;
    case '7': *vk_code = 0x37; return true; case '&': *need_shift = true; *vk_code = 0x37; return true;
    case '8': *vk_code = 0x38; return true; case '*': *need_shift = true; *vk_code = 0x38; return true;
    case '9': *vk_code = 0x39; return true; case '(': *need_shift = true; *vk_code = 0x39; return true;
    case '-': *vk_code = 0xBD; return true; case '_': *need_shift = true; *vk_code = 0xBD; return true;
    case '=': *vk_code = 0xBB; return true; case '+': *need_shift = true; *vk_code = 0xBB; return true;
    case '`': *vk_code = 0xC0; return true; case '~': *need_shift = true; *vk_code = 0xC0; return true;
    case '[': *vk_code = 0xDB; return true; case '{': *need_shift = true; *vk_code = 0xDB; return true;
    case ']': *vk_code = 0xDD; return true; case '}': *need_shift = true; *vk_code = 0xDD; return true;
    case '\\': *vk_code = 0xDC; return true; case '|': *need_shift = true; *vk_code = 0xDC; return true;
    case ';': *vk_code = 0xBA; return true; case ':': *need_shift = true; *vk_code = 0xBA; return true;
    case '\'': *vk_code = 0xDE; return true; case '"': *need_shift = true; *vk_code = 0xDE; return true;
    case ',': *vk_code = 0xBC; return true; case '<': *need_shift = true; *vk_code = 0xBC; return true;
    case '.': *vk_code = 0xBE; return true; case '>': *need_shift = true; *vk_code = 0xBE; return true;
    case '/': *vk_code = 0xBF; return true; case '?': *need_shift = true; *vk_code = 0xBF; return true;
    default: return false;
    }
    // clang-format on
}
#endif

YAError map_char_to_platform(int32_t codepoint, uint32_t* platform_code, bool* need_shift) {
#if defined(__APPLE__)
    if (map_char_to_platform_macos(codepoint, platform_code, need_shift)) {
        return Success;
    }
#elif defined(_WIN32)
    if (map_char_to_platform_windows(codepoint, platform_code, need_shift)) {
        return Success;
    }
#endif
    return NotFound;
}

#endif // USE_UINPUT