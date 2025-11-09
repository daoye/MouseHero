// Refactored key_inject.c - Pure character mapping, no modifier/clipboard logic
#include "input/keyboard/mapper.h"
#include "rs.h"
#include "ya_logger.h"
#include <stdint.h>
#include <string.h>

#ifdef _WIN32
#include <windows.h>
#endif

#ifdef __APPLE__
// macOS CGKeyCode for US layout (from HIToolbox/Events.h)
#define kVK_ANSI_A 0x00
#define kVK_ANSI_S 0x01
#define kVK_ANSI_D 0x02
#define kVK_ANSI_F 0x03
#define kVK_ANSI_H 0x04
#define kVK_ANSI_G 0x05
#define kVK_ANSI_Z 0x06
#define kVK_ANSI_X 0x07
#define kVK_ANSI_C 0x08
#define kVK_ANSI_V 0x09
#define kVK_ANSI_B 0x0B
#define kVK_ANSI_Q 0x0C
#define kVK_ANSI_W 0x0D
#define kVK_ANSI_E 0x0E
#define kVK_ANSI_R 0x0F
#define kVK_ANSI_Y 0x10
#define kVK_ANSI_T 0x11
#define kVK_ANSI_1 0x12
#define kVK_ANSI_2 0x13
#define kVK_ANSI_3 0x14
#define kVK_ANSI_4 0x15
#define kVK_ANSI_6 0x16
#define kVK_ANSI_5 0x17
#define kVK_ANSI_Equal 0x18
#define kVK_ANSI_9 0x19
#define kVK_ANSI_7 0x1A
#define kVK_ANSI_Minus 0x1B
#define kVK_ANSI_8 0x1C
#define kVK_ANSI_0 0x1D
#define kVK_ANSI_RightBracket 0x1E
#define kVK_ANSI_O 0x1F
#define kVK_ANSI_U 0x20
#define kVK_ANSI_LeftBracket 0x21
#define kVK_ANSI_I 0x22
#define kVK_ANSI_P 0x23
#define kVK_ANSI_L 0x25
#define kVK_ANSI_J 0x26
#define kVK_ANSI_Quote 0x27
#define kVK_ANSI_K 0x28
#define kVK_ANSI_Semicolon 0x29
#define kVK_ANSI_Backslash 0x2A
#define kVK_ANSI_Comma 0x2B
#define kVK_ANSI_Slash 0x2C
#define kVK_ANSI_N 0x2D
#define kVK_ANSI_M 0x2E
#define kVK_ANSI_Period 0x2F
#define kVK_ANSI_Grave 0x32
#define kVK_Space 0x31
#endif

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
    // Letters (a-z, A-Z)
    case 'a': case 'A': *platform_code = kVK_ANSI_A; *need_shift = (ch == 'A'); return true;
    case 'b': case 'B': *platform_code = kVK_ANSI_B; *need_shift = (ch == 'B'); return true;
    case 'c': case 'C': *platform_code = kVK_ANSI_C; *need_shift = (ch == 'C'); return true;
    case 'd': case 'D': *platform_code = kVK_ANSI_D; *need_shift = (ch == 'D'); return true;
    case 'e': case 'E': *platform_code = kVK_ANSI_E; *need_shift = (ch == 'E'); return true;
    case 'f': case 'F': *platform_code = kVK_ANSI_F; *need_shift = (ch == 'F'); return true;
    case 'g': case 'G': *platform_code = kVK_ANSI_G; *need_shift = (ch == 'G'); return true;
    case 'h': case 'H': *platform_code = kVK_ANSI_H; *need_shift = (ch == 'H'); return true;
    case 'i': case 'I': *platform_code = kVK_ANSI_I; *need_shift = (ch == 'I'); return true;
    case 'j': case 'J': *platform_code = kVK_ANSI_J; *need_shift = (ch == 'J'); return true;
    case 'k': case 'K': *platform_code = kVK_ANSI_K; *need_shift = (ch == 'K'); return true;
    case 'l': case 'L': *platform_code = kVK_ANSI_L; *need_shift = (ch == 'L'); return true;
    case 'm': case 'M': *platform_code = kVK_ANSI_M; *need_shift = (ch == 'M'); return true;
    case 'n': case 'N': *platform_code = kVK_ANSI_N; *need_shift = (ch == 'N'); return true;
    case 'o': case 'O': *platform_code = kVK_ANSI_O; *need_shift = (ch == 'O'); return true;
    case 'p': case 'P': *platform_code = kVK_ANSI_P; *need_shift = (ch == 'P'); return true;
    case 'q': case 'Q': *platform_code = kVK_ANSI_Q; *need_shift = (ch == 'Q'); return true;
    case 'r': case 'R': *platform_code = kVK_ANSI_R; *need_shift = (ch == 'R'); return true;
    case 's': case 'S': *platform_code = kVK_ANSI_S; *need_shift = (ch == 'S'); return true;
    case 't': case 'T': *platform_code = kVK_ANSI_T; *need_shift = (ch == 'T'); return true;
    case 'u': case 'U': *platform_code = kVK_ANSI_U; *need_shift = (ch == 'U'); return true;
    case 'v': case 'V': *platform_code = kVK_ANSI_V; *need_shift = (ch == 'V'); return true;
    case 'w': case 'W': *platform_code = kVK_ANSI_W; *need_shift = (ch == 'W'); return true;
    case 'x': case 'X': *platform_code = kVK_ANSI_X; *need_shift = (ch == 'X'); return true;
    case 'y': case 'Y': *platform_code = kVK_ANSI_Y; *need_shift = (ch == 'Y'); return true;
    case 'z': case 'Z': *platform_code = kVK_ANSI_Z; *need_shift = (ch == 'Z'); return true;
    
    // Digits and symbols
    case ' ': *platform_code = kVK_Space; return true;
    case '0': *platform_code = kVK_ANSI_0; return true; case ')': *need_shift = true; *platform_code = kVK_ANSI_0; return true;
    case '1': *platform_code = kVK_ANSI_1; return true; case '!': *need_shift = true; *platform_code = kVK_ANSI_1; return true;
    case '2': *platform_code = kVK_ANSI_2; return true; case '@': *need_shift = true; *platform_code = kVK_ANSI_2; return true;
    case '3': *platform_code = kVK_ANSI_3; return true; case '#': *need_shift = true; *platform_code = kVK_ANSI_3; return true;
    case '4': *platform_code = kVK_ANSI_4; return true; case '$': *need_shift = true; *platform_code = kVK_ANSI_4; return true;
    case '5': *platform_code = kVK_ANSI_5; return true; case '%': *need_shift = true; *platform_code = kVK_ANSI_5; return true;
    case '6': *platform_code = kVK_ANSI_6; return true; case '^': *need_shift = true; *platform_code = kVK_ANSI_6; return true;
    case '7': *platform_code = kVK_ANSI_7; return true; case '&': *need_shift = true; *platform_code = kVK_ANSI_7; return true;
    case '8': *platform_code = kVK_ANSI_8; return true; case '*': *need_shift = true; *platform_code = kVK_ANSI_8; return true;
    case '9': *platform_code = kVK_ANSI_9; return true; case '(': *need_shift = true; *platform_code = kVK_ANSI_9; return true;
    case '-': *platform_code = kVK_ANSI_Minus; return true; case '_': *need_shift = true; *platform_code = kVK_ANSI_Minus; return true;
    case '=': *platform_code = kVK_ANSI_Equal; return true; case '+': *need_shift = true; *platform_code = kVK_ANSI_Equal; return true;
    case '`': *platform_code = kVK_ANSI_Grave; return true; case '~': *need_shift = true; *platform_code = kVK_ANSI_Grave; return true;
    case '[': *platform_code = kVK_ANSI_LeftBracket; return true; case '{': *need_shift = true; *platform_code = kVK_ANSI_LeftBracket; return true;
    case ']': *platform_code = kVK_ANSI_RightBracket; return true; case '}': *need_shift = true; *platform_code = kVK_ANSI_RightBracket; return true;
    case '\\': *platform_code = kVK_ANSI_Backslash; return true; case '|': *need_shift = true; *platform_code = kVK_ANSI_Backslash; return true;
    case ';': *platform_code = kVK_ANSI_Semicolon; return true; case ':': *need_shift = true; *platform_code = kVK_ANSI_Semicolon; return true;
    case '\'': *platform_code = kVK_ANSI_Quote; return true; case '"': *need_shift = true; *platform_code = kVK_ANSI_Quote; return true;
    case ',': *platform_code = kVK_ANSI_Comma; return true; case '<': *need_shift = true; *platform_code = kVK_ANSI_Comma; return true;
    case '.': *platform_code = kVK_ANSI_Period; return true; case '>': *need_shift = true; *platform_code = kVK_ANSI_Period; return true;
    case '/': *platform_code = kVK_ANSI_Slash; return true; case '?': *need_shift = true; *platform_code = kVK_ANSI_Slash; return true;
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
    // Letters (a-z, A-Z) - VK codes are uppercase ASCII (0x41-0x5A)
    case 'a': case 'A': *vk_code = 'A'; *need_shift = (ch == 'A'); return true;
    case 'b': case 'B': *vk_code = 'B'; *need_shift = (ch == 'B'); return true;
    case 'c': case 'C': *vk_code = 'C'; *need_shift = (ch == 'C'); return true;
    case 'd': case 'D': *vk_code = 'D'; *need_shift = (ch == 'D'); return true;
    case 'e': case 'E': *vk_code = 'E'; *need_shift = (ch == 'E'); return true;
    case 'f': case 'F': *vk_code = 'F'; *need_shift = (ch == 'F'); return true;
    case 'g': case 'G': *vk_code = 'G'; *need_shift = (ch == 'G'); return true;
    case 'h': case 'H': *vk_code = 'H'; *need_shift = (ch == 'H'); return true;
    case 'i': case 'I': *vk_code = 'I'; *need_shift = (ch == 'I'); return true;
    case 'j': case 'J': *vk_code = 'J'; *need_shift = (ch == 'J'); return true;
    case 'k': case 'K': *vk_code = 'K'; *need_shift = (ch == 'K'); return true;
    case 'l': case 'L': *vk_code = 'L'; *need_shift = (ch == 'L'); return true;
    case 'm': case 'M': *vk_code = 'M'; *need_shift = (ch == 'M'); return true;
    case 'n': case 'N': *vk_code = 'N'; *need_shift = (ch == 'N'); return true;
    case 'o': case 'O': *vk_code = 'O'; *need_shift = (ch == 'O'); return true;
    case 'p': case 'P': *vk_code = 'P'; *need_shift = (ch == 'P'); return true;
    case 'q': case 'Q': *vk_code = 'Q'; *need_shift = (ch == 'Q'); return true;
    case 'r': case 'R': *vk_code = 'R'; *need_shift = (ch == 'R'); return true;
    case 's': case 'S': *vk_code = 'S'; *need_shift = (ch == 'S'); return true;
    case 't': case 'T': *vk_code = 'T'; *need_shift = (ch == 'T'); return true;
    case 'u': case 'U': *vk_code = 'U'; *need_shift = (ch == 'U'); return true;
    case 'v': case 'V': *vk_code = 'V'; *need_shift = (ch == 'V'); return true;
    case 'w': case 'W': *vk_code = 'W'; *need_shift = (ch == 'W'); return true;
    case 'x': case 'X': *vk_code = 'X'; *need_shift = (ch == 'X'); return true;
    case 'y': case 'Y': *vk_code = 'Y'; *need_shift = (ch == 'Y'); return true;
    case 'z': case 'Z': *vk_code = 'Z'; *need_shift = (ch == 'Z'); return true;
    
    // Digits (0x30-0x39) and symbols
    case ' ': *vk_code = VK_SPACE; return true;
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
    case '-': *vk_code = VK_OEM_MINUS; return true; case '_': *need_shift = true; *vk_code = VK_OEM_MINUS; return true;
    case '=': *vk_code = VK_OEM_PLUS; return true; case '+': *need_shift = true; *vk_code = VK_OEM_PLUS; return true;
    case '`': *vk_code = VK_OEM_3; return true; case '~': *need_shift = true; *vk_code = VK_OEM_3; return true;
    case '[': *vk_code = VK_OEM_4; return true; case '{': *need_shift = true; *vk_code = VK_OEM_4; return true;
    case ']': *vk_code = VK_OEM_6; return true; case '}': *need_shift = true; *vk_code = VK_OEM_6; return true;
    case '\\': *vk_code = VK_OEM_5; return true; case '|': *need_shift = true; *vk_code = VK_OEM_5; return true;
    case ';': *vk_code = VK_OEM_1; return true; case ':': *need_shift = true; *vk_code = VK_OEM_1; return true;
    case '\'': *vk_code = VK_OEM_7; return true; case '"': *need_shift = true; *vk_code = VK_OEM_7; return true;
    case ',': *vk_code = VK_OEM_COMMA; return true; case '<': *need_shift = true; *vk_code = VK_OEM_COMMA; return true;
    case '.': *vk_code = VK_OEM_PERIOD; return true; case '>': *need_shift = true; *vk_code = VK_OEM_PERIOD; return true;
    case '/': *vk_code = VK_OEM_2; return true; case '?': *need_shift = true; *vk_code = VK_OEM_2; return true;
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