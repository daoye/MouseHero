#include "keycode_map.h"

// YA_* 协议码到 CKey 的映射（非 ASCII 键）。
// 注意：所有非 ASCII 键均须使用 YA_* 协议码；ASCII 在服务端不再做基键推断。

// 功能按键
static bool map_ya_to_ckey(int32_t code, enum CKey *out) {
    if (!out) return false;
    // 保留的 macOS Fn
    if (code == 0xE000) { *out = Function; return true; }

    // 修饰与系统（通用 + 左右分键）
    switch (code) {
        case 0xE200: *out = Shift; return true;
        case 0xE201: *out = Control; return true;
        case 0xE202: *out = Alt; return true;
        case 0xE203: *out = Meta; return true;
        case 0xE204: *out = Apps; return true;
        case 0xE210: *out = LShift; return true;
        case 0xE211: *out = RShift; return true;
        case 0xE212: *out = LControl; return true;
        case 0xE213: *out = RControl; return true;
        case 0xE214: *out = LMenu; return true;
        case 0xE215: *out = RMenu; return true;
        default: break;
    }

    // 基本编辑/控制
    switch (code) {
        case 0xE220: *out = Backspace; return true;
        case 0xE221: *out = Tab; return true;
        case 0xE222: *out = Return; return true;
        case 0xE223: *out = Escape_k; return true;
        case 0xE224: *out = Insert; return true;
        case 0xE225: *out = Delete; return true;
        default: break;
    }

    // 导航与方向
    switch (code) {
        case 0xE230: *out = Home; return true;
        case 0xE231: *out = End; return true;
        case 0xE232: *out = PageUp; return true;
        case 0xE233: *out = PageDown; return true;
        case 0xE234: *out = LeftArrow; return true;
        case 0xE235: *out = UpArrow; return true;
        case 0xE236: *out = RightArrow; return true;
        case 0xE237: *out = DownArrow; return true;
        default: break;
    }

    // 锁定/屏幕/暂停
    switch (code) {
        case 0xE240: *out = CapsLock; return true;
        case 0xE241: *out = Numlock; return true;
        case 0xE242: *out = ScrollLock; return true;
        case 0xE243: *out = PrintScr; return true;
        case 0xE244: *out = Pause; return true;
        default: break;
    }

    // 功能键 F1..F24（连续） 0xE300..0xE317
    if (code >= 0xE300 && code <= 0xE317) {
        *out = (enum CKey)(F1 + (code - 0xE300));
        return true;
    }

    // Numpad0..9（连续） 0xE400..0xE409
    if (code >= 0xE400 && code <= 0xE409) {
        *out = (enum CKey)(Numpad0 + (code - 0xE400));
        return true;
    }

    // Numpad 运算符 0xE40A..0xE40F
    switch (code) {
        case 0xE40A: *out = Multiply; return true;
        case 0xE40B: *out = Add; return true;
        case 0xE40C: *out = Separator; return true;
        case 0xE40D: *out = Subtract; return true;
        case 0xE40E: *out = Decimal; return true;
        case 0xE40F: *out = Divide; return true;
        default: break;
    }

    // 浏览器键 0xE260..0xE266（连续）
    if (code >= 0xE260 && code <= 0xE266) {
        switch (code) {
            case 0xE260: *out = BrowserBack; return true;
            case 0xE261: *out = BrowserForward; return true;
            case 0xE262: *out = BrowserRefresh; return true;
            case 0xE263: *out = BrowserStop; return true;
            case 0xE264: *out = BrowserSearch; return true;
            case 0xE265: *out = BrowserFavorites; return true;
            case 0xE266: *out = BrowserHome; return true;
        }
    }

    // 媒体与音量 0xE270..0xE276
    if (code >= 0xE270 && code <= 0xE276) {
        switch (code) {
            case 0xE270: *out = MediaNextTrack; return true;
            case 0xE271: *out = MediaPrevTrack; return true;
            case 0xE272: *out = MediaStop; return true;
            case 0xE273: *out = MediaPlayPause; return true;
            case 0xE274: *out = VolumeMute; return true;
            case 0xE275: *out = VolumeDown; return true;
            case 0xE276: *out = VolumeUp; return true;
        }
    }

    // 启动类 0xE280..0xE283
    if (code >= 0xE280 && code <= 0xE283) {
        switch (code) {
            case 0xE280: *out = LaunchMail; return true;
            case 0xE281: *out = LaunchMediaSelect; return true;
            case 0xE282: *out = LaunchApp1; return true;
            case 0xE283: *out = LaunchApp2; return true;
        }
    }

    return false;
}

bool ckey_from_protocol(int32_t code, enum CKey *out) {
    if (!out) return false;
    return map_ya_to_ckey(code, out);
}
