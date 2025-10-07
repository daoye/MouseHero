#pragma once

#include <stdbool.h>


// mouse throttle used to batch relative moves before calling move_mouse().
void ya_mouse_throttle_reset(void);

// Accumulate delta. When ready to emit (interval elapsed or force_flush=true), returns true and
// writes aggregated delta into out_dx/out_dy. When not ready, returns false and leaves outputs untouched.
bool ya_mouse_throttle_collect(int dx, int dy, bool force_flush, int *out_dx, int *out_dy);