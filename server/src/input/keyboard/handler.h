#pragma once

#include <stdint.h>
#include <stdbool.h>
#include "rs.h"

#ifdef __cplusplus
extern "C" {
#endif

/**
 * Keyboard character input handler
 * @param codepoint Unicode codepoint
 * @param mods Modifier bitmask (CHORD_MOD_SHIFT | CHORD_MOD_CTRL | ...)
 * @param dir Direction (Press, Release, Click)
 * @return YAError status code
 */
YAError keyboard_char(int32_t codepoint, uint32_t mods, enum CDirection dir);

/**
 * Keyboard function key handler
 * @param key Function key (CKey enum)
 * @param mods Modifier bitmask (CHORD_MOD_SHIFT | CHORD_MOD_CTRL | ...)
 * @param dir Direction (Press, Release, Click)
 * @return YAError status code
 */
YAError keyboard_function_key(enum CKey key, uint32_t mods, enum CDirection dir);

#ifdef __cplusplus
}
#endif
