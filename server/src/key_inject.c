// 必要的头文件
#include "key_inject.h"
#include "rs.h"
#include <stdint.h>

static inline YAError physical_combo(enum CKey base, bool need_shift, enum CDirection dir)
{
    if (dir == Click)
    {
        bool shift_pressed = false;
        if (need_shift)
        {
            (void)key_action(Shift, Press);
            shift_pressed = true;
        }
        YAError e1 = key_action(base, Press);
        if (e1 != Success)
        {
            if (shift_pressed)
            {
                (void)key_action(Shift, Release);
            }
            return e1;
        }
        YAError e2 = key_action(base, Release);
        if (shift_pressed)
        {
            (void)key_action(Shift, Release);
        }
        return e2;
    }
    else if (dir == Press)
    {
        bool shift_pressed = false;
        if (need_shift)
        {
            (void)key_action(Shift, Press);
            shift_pressed = true;
        }
        YAError e = key_action(base, Press);
        if (e != Success && shift_pressed)
        {
            (void)key_action(Shift, Release);
        }
        return e;
    }
    else
    { // Release
        YAError e = key_action(base, Release);
        if (need_shift)
        {
            (void)key_action(Shift, Release);
        }
        return e;
    }
}

static inline YAError platform_physical_combo(uint32_t platform_code, bool need_shift, enum CDirection dir)
{
    if (dir == Click)
    {
        bool shift_pressed = false;
        if (need_shift)
        {
            (void)key_action(Shift, Press);
            shift_pressed = true;
        }
        YAError e1 = key_action_with_platform_code(platform_code, Press);
        if (e1 != Success)
        {
            if (shift_pressed)
            {
                (void)key_action(Shift, Release);
            }
            return e1;
        }
        YAError e2 = key_action_with_platform_code(platform_code, Release);
        if (shift_pressed)
        {
            (void)key_action(Shift, Release);
        }
        return e2;
    }
    else if (dir == Press)
    {
        bool shift_pressed = false;
        if (need_shift)
        {
            (void)key_action(Shift, Press);
            shift_pressed = true;
        }
        YAError e = key_action_with_platform_code(platform_code, Press);
        if (e != Success && shift_pressed)
        {
            (void)key_action(Shift, Release);
        }
        return e;
    }
    else // Release
    {
        YAError e = key_action_with_platform_code(platform_code, Release);
        if (need_shift)
        {
            (void)key_action(Shift, Release);
        }
        return e;
    }
}

static inline YAError unicode_combo(uint32_t base_uc, bool need_shift, enum CDirection dir)
{
    if (dir == Click)
    {
        bool shift_pressed = false;
        if (need_shift)
        {
            (void)key_action(Shift, Press);
            shift_pressed = true;
        }
        YAError e1 = key_unicode_action(base_uc, Press);
        if (e1 != Success)
        {
            if (shift_pressed)
            {
                (void)key_action(Shift, Release);
            }
            return e1;
        }
        YAError e2 = key_unicode_action(base_uc, Release);
        if (shift_pressed)
        {
            (void)key_action(Shift, Release);
        }
        return e2;
    }
    else if (dir == Press)
    {
        bool shift_pressed = false;
        if (need_shift)
        {
            (void)key_action(Shift, Press);
            shift_pressed = true;
        }
        YAError e = key_unicode_action(base_uc, Press);
        if (e != Success && shift_pressed)
        {
            (void)key_action(Shift, Release);
        }
        return e;
    }
    else
    { // Release
        YAError e = key_unicode_action(base_uc, Release);
        if (need_shift)
        {
            (void)key_action(Shift, Release);
        }
        return e;
    }
}

// macOS: US 布局 ASCII -> CGKeyCode 映射（物理键码）。
// 说明：同一键位的 Shift 变体复用相同 keycode，仅通过 need_shift 标记区分。
#if defined(__APPLE__)
static inline bool map_to_platform_code_us_macos(int32_t ch, uint32_t *platform_code, bool *need_shift)
{
    if (!platform_code || !need_shift) return false;
    *need_shift = false;

    // clang-format off
    switch (ch)
    {
    // 数字行 1..0
    case '1': *platform_code = 0x12; return true; // kVK_ANSI_1
    case '2': *platform_code = 0x13; return true; // kVK_ANSI_2
    case '3': *platform_code = 0x14; return true; // kVK_ANSI_3
    case '4': *platform_code = 0x15; return true; // kVK_ANSI_4
    case '5': *platform_code = 0x17; return true; // kVK_ANSI_5
    case '6': *platform_code = 0x16; return true; // kVK_ANSI_6
    case '7': *platform_code = 0x1A; return true; // kVK_ANSI_7
    case '8': *platform_code = 0x1C; return true; // kVK_ANSI_8
    case '9': *platform_code = 0x19; return true; // kVK_ANSI_9
    case '0': *platform_code = 0x1D; return true; // kVK_ANSI_0

    // 符号（同一键位，Shift 变体）
    case '!': *need_shift = true; *platform_code = 0x12; return true;
    case '@': *need_shift = true; *platform_code = 0x13; return true;
    case '#': *need_shift = true; *platform_code = 0x14; return true;
    case '$': *need_shift = true; *platform_code = 0x15; return true;
    case '%': *need_shift = true; *platform_code = 0x17; return true;
    case '^': *need_shift = true; *platform_code = 0x16; return true;
    case '&': *need_shift = true; *platform_code = 0x1A; return true;
    case '*': *need_shift = true; *platform_code = 0x1C; return true;
    case '(': *need_shift = true; *platform_code = 0x19; return true;
    case ')': *need_shift = true; *platform_code = 0x1D; return true;

    // 顶部符号键
    case '-': *platform_code = 0x1B; return true; // kVK_ANSI_Minus
    case '_': *need_shift = true; *platform_code = 0x1B; return true;
    case '=': *platform_code = 0x18; return true; // kVK_ANSI_Equal
    case '+': *need_shift = true; *platform_code = 0x18; return true;
    case '`': *platform_code = 0x32; return true; // kVK_ANSI_Grave
    case '~': *need_shift = true; *platform_code = 0x32; return true;

    // 中排符号键
    case '[': *platform_code = 0x21; return true; // kVK_ANSI_LeftBracket
    case '{': *need_shift = true; *platform_code = 0x21; return true;
    case ']': *platform_code = 0x1E; return true; // kVK_ANSI_RightBracket
    case '}': *need_shift = true; *platform_code = 0x1E; return true;
    case '\\': *platform_code = 0x2A; return true; // kVK_ANSI_Backslash
    case '|': *need_shift = true; *platform_code = 0x2A; return true;
    case ';': *platform_code = 0x29; return true; // kVK_ANSI_Semicolon
    case ':': *need_shift = true; *platform_code = 0x29; return true;
    case '\'': *platform_code = 0x27; return true; // kVK_ANSI_Quote
    case '"': *need_shift = true; *platform_code = 0x27; return true;

    // 下排符号键
    case ',': *platform_code = 0x2B; return true; // kVK_ANSI_Comma
    case '<': *need_shift = true; *platform_code = 0x2B; return true;
    case '.': *platform_code = 0x2F; return true; // kVK_ANSI_Period
    case '>': *need_shift = true; *platform_code = 0x2F; return true;
    case '/': *platform_code = 0x2C; return true; // kVK_ANSI_Slash
    case '?': *need_shift = true; *platform_code = 0x2C; return true;

    default:
        return false;
    }
    // clang-format on
}
#endif // __APPLE__

// Windows: US 布局 ASCII -> Virtual-Key 映射（物理键码传入 enigo Other）。
#if defined(_WIN32)
static inline bool map_to_platform_code_us_windows(int32_t ch, uint32_t *vk_code, bool *need_shift)
{
    if (!vk_code || !need_shift) return false;
    *need_shift = false;

    // clang-format off
    switch (ch)
    {
    // 数字行 0..9
    case '0': *vk_code = 0x30; return true; // '0'
    case '1': *vk_code = 0x31; return true; // '1'
    case '2': *vk_code = 0x32; return true; // '2'
    case '3': *vk_code = 0x33; return true; // '3'
    case '4': *vk_code = 0x34; return true; // '4'
    case '5': *vk_code = 0x35; return true; // '5'
    case '6': *vk_code = 0x36; return true; // '6'
    case '7': *vk_code = 0x37; return true; // '7'
    case '8': *vk_code = 0x38; return true; // '8'
    case '9': *vk_code = 0x39; return true; // '9'

    // Shift 变体
    case ')': *need_shift = true; *vk_code = 0x30; return true;
    case '!': *need_shift = true; *vk_code = 0x31; return true;
    case '@': *need_shift = true; *vk_code = 0x32; return true;
    case '#': *need_shift = true; *vk_code = 0x33; return true;
    case '$': *need_shift = true; *vk_code = 0x34; return true;
    case '%': *need_shift = true; *vk_code = 0x35; return true;
    case '^': *need_shift = true; *vk_code = 0x36; return true;
    case '&': *need_shift = true; *vk_code = 0x37; return true;
    case '*': *need_shift = true; *vk_code = 0x38; return true;
    case '(': *need_shift = true; *vk_code = 0x39; return true;

    // 顶部符号键
    case '-': *vk_code = 0xBD; return true; // VK_OEM_MINUS
    case '_': *need_shift = true; *vk_code = 0xBD; return true;
    case '=': *vk_code = 0xBB; return true; // VK_OEM_PLUS
    case '+': *need_shift = true; *vk_code = 0xBB; return true;
    case '`': *vk_code = 0xC0; return true; // VK_OEM_3
    case '~': *need_shift = true; *vk_code = 0xC0; return true;

    // 中排符号键
    case '[': *vk_code = 0xDB; return true; // VK_OEM_4
    case '{': *need_shift = true; *vk_code = 0xDB; return true;
    case ']': *vk_code = 0xDD; return true; // VK_OEM_6
    case '}': *need_shift = true; *vk_code = 0xDD; return true;
    case '\\': *vk_code = 0xDC; return true; // VK_OEM_5
    case '|': *need_shift = true; *vk_code = 0xDC; return true;
    case ';': *vk_code = 0xBA; return true; // VK_OEM_1
    case ':': *need_shift = true; *vk_code = 0xBA; return true;
    case '\'': *vk_code = 0xDE; return true; // VK_OEM_7
    case '"': *need_shift = true; *vk_code = 0xDE; return true;

    // 下排符号键
    case ',': *vk_code = 0xBC; return true; // VK_OEM_COMMA
    case '<': *need_shift = true; *vk_code = 0xBC; return true;
    case '.': *vk_code = 0xBE; return true; // VK_OEM_PERIOD
    case '>': *need_shift = true; *vk_code = 0xBE; return true;
    case '/': *vk_code = 0xBF; return true; // VK_OEM_2
    case '?': *need_shift = true; *vk_code = 0xBF; return true;

    default:
        return false;
    }

    // clang-format on
}
#endif // _WIN32

// Linux: US 布局 ASCII -> X11 Keysym（传入 enigo 的 Key::Other 即为 keysym）。
#if defined(__linux__)
static inline bool map_to_platform_code_us_linux(int32_t ch, uint32_t *linux_code, bool *need_shift)
{
    if (!linux_code || !need_shift) return false;
    *need_shift = false;
    // 仅处理可打印 ASCII 0x20..0x7E
    if (ch < 0x20 || ch > 0x7E) return false;

    // clang-format off
    switch (ch)
    {
    // 空格与直接字符
    case ' ': *linux_code = ' '; return true;

    // 数字行及其 Shift 变体（基础 keysym 为未按 Shift 的字符）
    case '0': *linux_code = '0'; return true; case ')': *linux_code = '0'; *need_shift = true; return true;
    case '1': *linux_code = '1'; return true; case '!': *linux_code = '1'; *need_shift = true; return true;
    case '2': *linux_code = '2'; return true; case '@': *linux_code = '2'; *need_shift = true; return true;
    case '3': *linux_code = '3'; return true; case '#': *linux_code = '3'; *need_shift = true; return true;
    case '4': *linux_code = '4'; return true; case '$': *linux_code = '4'; *need_shift = true; return true;
    case '5': *linux_code = '5'; return true; case '%': *linux_code = '5'; *need_shift = true; return true;
    case '6': *linux_code = '6'; return true; case '^': *linux_code = '6'; *need_shift = true; return true;
    case '7': *linux_code = '7'; return true; case '&': *linux_code = '7'; *need_shift = true; return true;
    case '8': *linux_code = '8'; return true; case '*': *linux_code = '8'; *need_shift = true; return true;
    case '9': *linux_code = '9'; return true; case '(': *linux_code = '9'; *need_shift = true; return true;

    // 顶部符号键及 Shift 变体
    case '-': *linux_code = '-'; return true; case '_': *linux_code = '-'; *need_shift = true; return true;
    case '=': *linux_code = '='; return true; case '+': *linux_code = '='; *need_shift = true; return true;
    case '`': *linux_code = '`'; return true; case '~': *linux_code = '`'; *need_shift = true; return true;

    // 中排符号键
    case '[': *linux_code = '['; return true; case '{': *linux_code = '['; *need_shift = true; return true;
    case ']': *linux_code = ']'; return true; case '}': *linux_code = ']'; *need_shift = true; return true;
    case '\\': *linux_code = '\\'; return true; case '|': *linux_code = '\\'; *need_shift = true; return true;
    case ';': *linux_code = ';'; return true; case ':': *linux_code = ';'; *need_shift = true; return true;
    case '\'': *linux_code = '\''; return true; case '"': *linux_code = '\''; *need_shift = true; return true;

    // 下排符号键
    case ',': *linux_code = ','; return true; case '<': *linux_code = ','; *need_shift = true; return true;
    case '.': *linux_code = '.'; return true; case '>': *linux_code = '.'; *need_shift = true; return true;
    case '/': *linux_code = '/'; return true; case '?': *linux_code = '/'; *need_shift = true; return true;

    // 字母：统一用小写作为基础，若为大写则标记 Shift
    case 'a': case 'b': case 'c': case 'd': case 'e': case 'f': case 'g': case 'h':
    case 'i': case 'j': case 'k': case 'l': case 'm': case 'n': case 'o': case 'p':
    case 'q': case 'r': case 's': case 't': case 'u': case 'v': case 'w': case 'x':
    case 'y': case 'z':
        *linux_code = (uint32_t)ch; return true;
    case 'A': case 'B': case 'C': case 'D': case 'E': case 'F': case 'G': case 'H':
    case 'I': case 'J': case 'K': case 'L': case 'M': case 'N': case 'O': case 'P':
    case 'Q': case 'R': case 'S': case 'T': case 'U': case 'V': case 'W': case 'X':
    case 'Y': case 'Z':
        *linux_code = (uint32_t)(ch + ('a' - 'A')); *need_shift = true; return true;

    default:
        return false;
    }
    // clang-format on
}
#endif // __linux__

// US 布局：将 ASCII 字符映射为“基础 Unicode 字符 + 是否需要 Shift”
// 例如：'A' -> 'a' + Shift，'!' -> '1' + Shift，'_' -> '-' + Shift
static inline bool map_ascii_to_unicode_base_us(int32_t ch, uint32_t *base_uc, bool *need_shift)
{
    if (!base_uc || !need_shift) return false;
    *need_shift = false;
    // 仅处理可打印 ASCII 0x20..0x7E
    if (ch < 0x20 || ch > 0x7E) return false;

    // clang-format off
    switch (ch)
    {
    // 空格与直接字符
    case ' ': *base_uc = ' '; return true;

    // 数字行及其 Shift 变体
    case '0': *base_uc = '0'; return true; case ')': *base_uc = '0'; *need_shift = true; return true;
    case '1': *base_uc = '1'; return true; case '!': *base_uc = '1'; *need_shift = true; return true;
    case '2': *base_uc = '2'; return true; case '@': *base_uc = '2'; *need_shift = true; return true;
    case '3': *base_uc = '3'; return true; case '#': *base_uc = '3'; *need_shift = true; return true;
    case '4': *base_uc = '4'; return true; case '$': *base_uc = '4'; *need_shift = true; return true;
    case '5': *base_uc = '5'; return true; case '%': *base_uc = '5'; *need_shift = true; return true;
    case '6': *base_uc = '6'; return true; case '^': *base_uc = '6'; *need_shift = true; return true;
    case '7': *base_uc = '7'; return true; case '&': *base_uc = '7'; *need_shift = true; return true;
    case '8': *base_uc = '8'; return true; case '*': *base_uc = '8'; *need_shift = true; return true;
    case '9': *base_uc = '9'; return true; case '(': *base_uc = '9'; *need_shift = true; return true;

    // 顶部符号键及 Shift 变体
    case '-': *base_uc = '-'; return true; case '_': *base_uc = '-'; *need_shift = true; return true;
    case '=': *base_uc = '='; return true; case '+': *base_uc = '='; *need_shift = true; return true;
    case '`': *base_uc = '`'; return true; case '~': *base_uc = '`'; *need_shift = true; return true;

    // 中排符号键
    case '[': *base_uc = '['; return true; case '{': *base_uc = '['; *need_shift = true; return true;
    case ']': *base_uc = ']'; return true; case '}': *base_uc = ']'; *need_shift = true; return true;
    case '\\': *base_uc = '\\'; return true; case '|': *base_uc = '\\'; *need_shift = true; return true;
    case ';': *base_uc = ';'; return true; case ':': *base_uc = ';'; *need_shift = true; return true;
    case '\'': *base_uc = '\''; return true; case '"': *base_uc = '\''; *need_shift = true; return true;

    // 下排符号键
    case ',': *base_uc = ','; return true; case '<': *base_uc = ','; *need_shift = true; return true;
    case '.': *base_uc = '.'; return true; case '>': *base_uc = '.'; *need_shift = true; return true;
    case '/': *base_uc = '/'; return true; case '?': *base_uc = '/'; *need_shift = true; return true;

    // 字母：统一用小写作为基础，若为大写则标记 Shift
    case 'a': case 'b': case 'c': case 'd': case 'e': case 'f': case 'g': case 'h':
    case 'i': case 'j': case 'k': case 'l': case 'm': case 'n': case 'o': case 'p':
    case 'q': case 'r': case 's': case 't': case 'u': case 'v': case 'w': case 'x':
    case 'y': case 'z':
        *base_uc = (uint32_t)ch; return true;
    case 'A': case 'B': case 'C': case 'D': case 'E': case 'F': case 'G': case 'H':
    case 'I': case 'J': case 'K': case 'L': case 'M': case 'N': case 'O': case 'P':
    case 'Q': case 'R': case 'S': case 'T': case 'U': case 'V': case 'W': case 'X':
    case 'Y': case 'Z':
        *base_uc = (uint32_t)(ch + ('a' - 'A')); *need_shift = true; return true;

    default:
        return false;
    }
    // clang-format on
}

static inline bool map_ascii_to_ckey(int32_t code, enum CKey *base, bool *need_shift)
{
    if (!base || !need_shift) return false;
    *need_shift = false;

    // clang-format off
#if defined(_WIN32)
    switch (code)
    {
    case ' ': *base = Space; return true;
    // digits 0..9
    case '0': *base = Num0; return true;
    case '1': *base = Num1; return true;
    case '2': *base = Num2; return true;
    case '3': *base = Num3; return true;
    case '4': *base = Num4; return true;
    case '5': *base = Num5; return true;
    case '6': *base = Num6; return true;
    case '7': *base = Num7; return true;
    case '8': *base = Num8; return true;
    case '9': *base = Num9; return true;

    // letters
    case 'a': *base = A; return true; case 'A': *base = A; *need_shift = true; return true;
    case 'b': *base = B; return true; case 'B': *base = B; *need_shift = true; return true;
    case 'c': *base = C; return true; case 'C': *base = C; *need_shift = true; return true;
    case 'd': *base = D; return true; case 'D': *base = D; *need_shift = true; return true;
    case 'e': *base = E; return true; case 'E': *base = E; *need_shift = true; return true;
    case 'f': *base = F; return true; case 'F': *base = F; *need_shift = true; return true;
    case 'g': *base = G; return true; case 'G': *base = G; *need_shift = true; return true;
    case 'h': *base = H; return true; case 'H': *base = H; *need_shift = true; return true;
    case 'i': *base = I; return true; case 'I': *base = I; *need_shift = true; return true;
    case 'j': *base = J; return true; case 'J': *base = J; *need_shift = true; return true;
    case 'k': *base = K; return true; case 'K': *base = K; *need_shift = true; return true;
    case 'l': *base = L; return true; case 'L': *base = L; *need_shift = true; return true;
    case 'm': *base = M; return true; case 'M': *base = M; *need_shift = true; return true;
    case 'n': *base = N; return true; case 'N': *base = N; *need_shift = true; return true;
    case 'o': *base = O; return true; case 'O': *base = O; *need_shift = true; return true;
    case 'p': *base = P; return true; case 'P': *base = P; *need_shift = true; return true;
    case 'q': *base = Q; return true; case 'Q': *base = Q; *need_shift = true; return true;
    case 'r': *base = R; return true; case 'R': *base = R; *need_shift = true; return true;
    case 's': *base = S; return true; case 'S': *base = S; *need_shift = true; return true;
    case 't': *base = T; return true; case 'T': *base = T; *need_shift = true; return true;
    case 'u': *base = U; return true; case 'U': *base = U; *need_shift = true; return true;
    case 'v': *base = V; return true; case 'V': *base = V; *need_shift = true; return true;
    case 'w': *base = W; return true; case 'W': *base = W; *need_shift = true; return true;
    case 'x': *base = X; return true; case 'X': *base = X; *need_shift = true; return true;
    case 'y': *base = Y; return true; case 'Y': *base = Y; *need_shift = true; return true;
    case 'z': *base = Z; return true; case 'Z': *base = Z; *need_shift = true; return true;

    // OEM and punctuation
    case '-': *base = OEMMinus; return true;
    case '_': *base = OEMMinus; *need_shift = true; return true;
    case '=': *base = OEMPlus; return true;
    case '+': *base = OEMPlus; *need_shift = true; return true;
    case '[': *base = OEM4; return true; case '{': *base = OEM4; *need_shift = true; return true;
    case ']': *base = OEM6; return true; case '}': *base = OEM6; *need_shift = true; return true;
    case '\\': *base = OEM5; return true; case '|': *base = OEM5; *need_shift = true; return true;
    case ';': *base = OEM1; return true; case ':': *base = OEM1; *need_shift = true; return true;
    case '\'': *base = OEM7; return true; case '"': *base = OEM7; *need_shift = true; return true;
    case ',': *base = OEMComma; return true; case '<': *base = OEMComma; *need_shift = true; return true;
    case '.': *base = OEMPeriod; return true; case '>': *base = OEMPeriod; *need_shift = true; return true;
    case '/': *base = OEM2; return true; case '?': *base = OEM2; *need_shift = true; return true;
    case '`': *base = OEM3; return true; case '~': *base = OEM3; *need_shift = true; return true;
    default:
        return false;
    }
#else
    // macOS/Linux: 仅将空格映射为 CKey::Space，其余返回 false
    switch (code)
    {
    case ' ': *base = Space; return true;
    default: return false;
    }
#endif
    // clang-format on
}

YAError ya_inject_ascii(int32_t code, enum CDirection dir)
{
    enum CKey base = Space;
    bool need_shift = false;
    if (map_ascii_to_ckey(code, &base, &need_shift))
    {
        return physical_combo(base, need_shift, dir);
    }
    uint32_t platform_code = 0;
    bool platform_need_shift = false;

#if defined(__APPLE__)
    if (map_to_platform_code_us_macos(code, &platform_code, &platform_need_shift))
#elif defined(_WIN32)
    if (map_to_platform_code_us_windows(code, &platform_code, &platform_need_shift))
#elif defined(__linux__)
    if (map_to_platform_code_us_linux(code, &platform_code, &platform_need_shift))
#endif
    {
        return platform_physical_combo(platform_code, platform_need_shift, dir);
    }

    uint32_t base_uc = 0;
    bool unicode_need_shift = false;
    if (map_ascii_to_unicode_base_us(code, &base_uc, &unicode_need_shift))
    {
        return unicode_combo(base_uc, unicode_need_shift, dir);
    }

    return key_unicode_action(code, dir);
}