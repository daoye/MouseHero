#pragma once

#include <stdint.h>
#include "rs.h"

#ifdef __linux__
#ifdef __cplusplus
extern "C" {
#endif

// Move mouse using libxdo on Linux. Keeps keyboard and other inputs via enigo.
enum YAError ya_xdo_move_mouse(int32_t x, int32_t y, enum CCoordinate coord);

// Mouse button actions using libxdo on Linux (Press/Release/Click)
enum YAError ya_xdo_button(enum CButton button, enum CDirection dir);

#ifdef __cplusplus
}
#endif
#endif // __linux__
