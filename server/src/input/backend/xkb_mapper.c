#include "input/backend/xkb_mapper.h"
#include "ya_logger.h"

#include <stdlib.h>
#include <string.h>

#ifdef __linux__
#include <linux/input-event-codes.h>
#endif

#ifdef __linux__
#ifdef HAVE_LIBXKBCOMMON
#include <xkbcommon/xkbcommon.h>
#ifdef HAVE_LIBXKBCOMMON_X11
#include <xkbcommon/xkbcommon-x11.h>
#include <xcb/xcb.h>
#endif
#endif

// Modifier bit positions in our cache
#define MOD_SHIFT_BIT 0
#define MOD_LEVEL3_BIT 1  // AltGr
#define MOD_CTRL_BIT 2
#define MOD_META_BIT 3

// Modifier combination flags for iteration
#define MOD_COMBO_NONE 0
#define MOD_COMBO_SHIFT 1
#define MOD_COMBO_LEVEL3 2
#define MOD_COMBO_SHIFT_LEVEL3 3
#define MOD_COMBO_COUNT 4

// XKB to evdev keycode offset
#define XKB_KEYCODE_OFFSET 8

// Mapping cache entry
typedef struct {
    uint32_t codepoint;
    int evdev_key;
    unsigned mods_mask;
} xkb_cache_entry_t;

// Global state
static struct {
    bool initialized;
    char layout_source[64];
#ifdef HAVE_LIBXKBCOMMON
    struct xkb_context* ctx;
    struct xkb_keymap* keymap;
    struct xkb_state* state;
    xkb_cache_entry_t* cache;
    size_t cache_size;
    size_t cache_capacity;
#endif
} g_xkb = {0};

#ifdef HAVE_LIBXKBCOMMON

// Build reverse mapping cache: keycode + mods → codepoint
static void build_cache(void) {
    if (!g_xkb.keymap || !g_xkb.state) return;
    
    g_xkb.cache_capacity = 512;
    g_xkb.cache = calloc(g_xkb.cache_capacity, sizeof(xkb_cache_entry_t));
    if (!g_xkb.cache) {
        YA_LOG_ERROR("xkbmap: failed to allocate cache");
        return;
    }
    
    xkb_keycode_t min_kc = xkb_keymap_min_keycode(g_xkb.keymap);
    xkb_keycode_t max_kc = xkb_keymap_max_keycode(g_xkb.keymap);
    
    // Iterate keycodes and modifier combinations
    for (xkb_keycode_t kc = min_kc; kc <= max_kc; ++kc) {
        // Try different modifier combinations: none, Shift, Level3, Shift+Level3
        for (unsigned mods = MOD_COMBO_NONE; mods < MOD_COMBO_COUNT; ++mods) {
            xkb_mod_mask_t mod_mask = 0;
            if (mods & MOD_COMBO_SHIFT) {
                xkb_mod_index_t shift_idx = xkb_keymap_mod_get_index(g_xkb.keymap, XKB_MOD_NAME_SHIFT);
                if (shift_idx != XKB_MOD_INVALID) {
                    mod_mask |= (1 << shift_idx);
                }
            }
            if (mods & MOD_COMBO_LEVEL3) {
                // Level3 (AltGr) - try ISO_Level3_Shift or Mode_switch
                xkb_mod_index_t level3_idx = xkb_keymap_mod_get_index(g_xkb.keymap, "ISO_Level3_Shift");
                if (level3_idx == XKB_MOD_INVALID) {
                    level3_idx = xkb_keymap_mod_get_index(g_xkb.keymap, "Mode_switch");
                }
                if (level3_idx != XKB_MOD_INVALID) {
                    mod_mask |= (1 << level3_idx);
                }
            }
            
            xkb_state_update_mask(g_xkb.state, mod_mask, 0, 0, 0, 0, 0);
            xkb_keysym_t keysym = xkb_state_key_get_one_sym(g_xkb.state, kc);
            
            if (keysym != XKB_KEY_NoSymbol) {
                uint32_t cp = xkb_keysym_to_utf32(keysym);
                if (cp > 0 && cp < 0x110000) {
                    // Add to cache if not already present
                    bool found = false;
                    for (size_t i = 0; i < g_xkb.cache_size; ++i) {
                        if (g_xkb.cache[i].codepoint == cp) {
                            found = true;
                            break;
                        }
                    }
                    
                    if (!found && g_xkb.cache_size < g_xkb.cache_capacity) {
                        // Map evdev keycode (xkb keycode - XKB_KEYCODE_OFFSET)
                        int evdev_kc = (int)kc - XKB_KEYCODE_OFFSET;
                        
                        unsigned cache_mods = 0;
                        if (mods & MOD_COMBO_SHIFT) cache_mods |= (1 << MOD_SHIFT_BIT);
                        if (mods & MOD_COMBO_LEVEL3) cache_mods |= (1 << MOD_LEVEL3_BIT);
                        
                        g_xkb.cache[g_xkb.cache_size].codepoint = cp;
                        g_xkb.cache[g_xkb.cache_size].evdev_key = evdev_kc;
                        g_xkb.cache[g_xkb.cache_size].mods_mask = cache_mods;
                        
                        // Debug log for specific characters
                        if (cp == '*' || cp == '8') {
                            YA_LOG_DEBUG("xkb cache: U+%04X ('%c') → evdev=%d, mods=0x%X (xkb_kc=%d, xkb_mods=%u)",
                                        cp, (char)cp, evdev_kc, cache_mods, kc, mods);
                        }
                        
                        g_xkb.cache_size++;
                    }
                }
            }
        }
    }
    
    // Reset state
    xkb_state_update_mask(g_xkb.state, 0, 0, 0, 0, 0, 0);
    
    YA_LOG_INFO("xkbmap: built cache with %zu entries", g_xkb.cache_size);
}

#ifdef HAVE_LIBXKBCOMMON_X11
static bool try_init_from_x11(void) {
    const char* display_str = getenv("DISPLAY");
    if (!display_str || display_str[0] == '\0') {
        return false;
    }
    
    int screen_idx = 0;
    xcb_connection_t* conn = xcb_connect(display_str, &screen_idx);
    if (!conn || xcb_connection_has_error(conn)) {
        if (conn) xcb_disconnect(conn);
        return false;
    }
    
    int32_t device_id = xkb_x11_get_core_keyboard_device_id(conn);
    if (device_id == -1) {
        xcb_disconnect(conn);
        return false;
    }
    
    g_xkb.keymap = xkb_x11_keymap_new_from_device(g_xkb.ctx, conn, device_id, XKB_KEYMAP_COMPILE_NO_FLAGS);
    xcb_disconnect(conn);
    
    if (g_xkb.keymap) {
        snprintf(g_xkb.layout_source, sizeof(g_xkb.layout_source), "X11");
        YA_LOG_INFO("xkbmap: loaded layout from X11");
        return true;
    }
    
    return false;
}
#endif

static bool try_init_from_env(void) {
    const char* layout = getenv("XKB_DEFAULT_LAYOUT");
    const char* variant = getenv("XKB_DEFAULT_VARIANT");
    const char* options = getenv("XKB_DEFAULT_OPTIONS");
    
    if (!layout || layout[0] == '\0') {
        return false;
    }
    
    struct xkb_rule_names names = {
        .rules = NULL,
        .model = NULL,
        .layout = layout,
        .variant = variant,
        .options = options
    };
    
    g_xkb.keymap = xkb_keymap_new_from_names(g_xkb.ctx, &names, XKB_KEYMAP_COMPILE_NO_FLAGS);
    
    if (g_xkb.keymap) {
        snprintf(g_xkb.layout_source, sizeof(g_xkb.layout_source), "env:%s%s%s",
                 layout, variant ? "," : "", variant ? variant : "");
        YA_LOG_INFO("xkbmap: loaded layout from environment: %s", g_xkb.layout_source);
        return true;
    }
    
    return false;
}

static bool try_init_fallback_us(void) {
    struct xkb_rule_names names = {
        .rules = NULL,
        .model = NULL,
        .layout = "us",
        .variant = NULL,
        .options = NULL
    };
    
    g_xkb.keymap = xkb_keymap_new_from_names(g_xkb.ctx, &names, XKB_KEYMAP_COMPILE_NO_FLAGS);
    
    if (g_xkb.keymap) {
        snprintf(g_xkb.layout_source, sizeof(g_xkb.layout_source), "fallback:us");
        YA_LOG_INFO("xkbmap: loaded fallback layout: us");
        return true;
    }
    
    return false;
}

#endif // HAVE_LIBXKBCOMMON

int xkbmap_init_auto(void) {
#ifdef HAVE_LIBXKBCOMMON
    if (g_xkb.initialized) {
        YA_LOG_WARN("xkbmap: already initialized");
        return 0;
    }
    
    g_xkb.ctx = xkb_context_new(XKB_CONTEXT_NO_FLAGS);
    if (!g_xkb.ctx) {
        YA_LOG_ERROR("xkbmap: failed to create xkb context");
        return -1;
    }
    
    // Try detection priority: X11 → env → fallback us
    bool success = false;
    
#ifdef HAVE_LIBXKBCOMMON_X11
    success = try_init_from_x11();
    if (success) goto init_state;
#endif
    
    success = try_init_from_env();
    if (success) goto init_state;
    
    success = try_init_fallback_us();
    if (!success) {
        YA_LOG_ERROR("xkbmap: failed to load any keymap");
        xkb_context_unref(g_xkb.ctx);
        g_xkb.ctx = NULL;
        return -1;
    }
    
init_state:
    g_xkb.state = xkb_state_new(g_xkb.keymap);
    if (!g_xkb.state) {
        YA_LOG_ERROR("xkbmap: failed to create xkb state");
        xkb_keymap_unref(g_xkb.keymap);
        xkb_context_unref(g_xkb.ctx);
        g_xkb.keymap = NULL;
        g_xkb.ctx = NULL;
        return -1;
    }
    
    build_cache();
    g_xkb.initialized = true;
    
    return 0;
#else
    YA_LOG_WARN("xkbmap: libxkbcommon not available at build time");
    snprintf(g_xkb.layout_source, sizeof(g_xkb.layout_source), "unavailable");
    return -1;
#endif
}

void xkbmap_free(void) {
#ifdef HAVE_LIBXKBCOMMON
    if (!g_xkb.initialized) return;
    
    if (g_xkb.cache) {
        free(g_xkb.cache);
        g_xkb.cache = NULL;
    }
    g_xkb.cache_size = 0;
    g_xkb.cache_capacity = 0;
    
    if (g_xkb.state) {
        xkb_state_unref(g_xkb.state);
        g_xkb.state = NULL;
    }
    
    if (g_xkb.keymap) {
        xkb_keymap_unref(g_xkb.keymap);
        g_xkb.keymap = NULL;
    }
    
    if (g_xkb.ctx) {
        xkb_context_unref(g_xkb.ctx);
        g_xkb.ctx = NULL;
    
    // Decode UTF-8 to code point (simple single-byte or multi-byte)
    uint32_t cp = 0;
    const unsigned char* u = (const unsigned char*)utf8;
    
    if ((u[0] & 0x80) == 0) {
        cp = u[0];
    } else if ((u[0] & 0xE0) == 0xC0) {
        cp = ((u[0] & 0x1F) << 6) | (u[1] & 0x3F);
    } else if ((u[0] & 0xF0) == 0xE0) {
        cp = ((u[0] & 0x0F) << 12) | ((u[1] & 0x3F) << 6) | (u[2] & 0x3F);
    } else if ((u[0] & 0xF8) == 0xF0) {
        cp = ((u[0] & 0x07) << 18) | ((u[1] & 0x3F) << 12) | ((u[2] & 0x3F) << 6) | (u[3] & 0x3F);
    } else {
        return false;
    }
    
    // Search cache
    for (size_t i = 0; i < g_xkb.cache_size; ++i) {
        if (g_xkb.cache[i].codepoint == cp) {
            *evdev_key = g_xkb.cache[i].evdev_key;
            *mods_mask = g_xkb.cache[i].mods_mask;
            return true;
        }
    }
    
    return false; // Not found (dead key/compose/unmappable)
#else
    (void)utf8;
    (void)evdev_key;
    (void)mods_mask;
    return false;
#endif
}

const char* xkbmap_get_layout_source(void) {
    if (g_xkb.layout_source[0] != '\0') {
        return g_xkb.layout_source;
    }
    return "uninitialized";
}

#else // __linux__

int xkbmap_init_auto(void) {
    YA_LOG_WARN("xkbmap: unsupported platform");
    return -1;
}

void xkbmap_free(void) {}

bool xkbmap_map_utf8_to_evdev(const char* utf8, int* evdev_key, unsigned* mods_mask) {
    (void)utf8;
    (void)evdev_key;
    (void)mods_mask;
    return false;
}

const char* xkbmap_get_layout_source(void) {
    return "unsupported";
}

#endif // __linux__
