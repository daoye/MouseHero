#ifndef INPUT_FACADE_H
#define INPUT_FACADE_H

#include "rs.h"
#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

/**
 * Input Service API - Platform-agnostic facade for keyboard/mouse input
 * 
 * This service provides a unified interface for input handling across platforms:
 * - Linux: delegates to input_backend (uinput)
 * - macOS/Windows: delegates to RS/Enigo
 * 
 * All functions return YAError for consistent error handling.
 */

/**
 * Move mouse cursor by relative offset
 * @param dx Horizontal movement in pixels (positive = right)
 * @param dy Vertical movement in pixels (positive = down)
 * @return Success or error code
 */
YAError input_mouse_move(int dx, int dy);

/**
 * Perform mouse button action
 * @param btn Button to act on (Left, Right, Middle, etc.)
 * @param dir Direction (Press, Release, Click)
 * @return Success or error code
 */
YAError input_mouse_button(enum CButton btn, enum CDirection dir);

/**
 * Scroll mouse wheel
 * @param amount Scroll distance (positive integer)
 * @param dir Direction: 0=up, 1=down, 2=left, 3=right
 * @return Success or error code
 */
YAError input_mouse_scroll(int amount, int dir);

/**
 * Perform keyboard key action
 * @param key Key to act on (CKey enum)
 * @param dir Direction (Press, Release, Click)
 * @return Success or error code
 */
YAError input_key_action(enum CKey key, enum CDirection dir);

#ifdef __cplusplus
}
#endif

#endif // INPUT_FACADE_H
