#include "ya_mouse_throttle.h"

#include <time.h>

typedef struct
{
    struct timespec last_emit;
    bool has_last_emit;
    int accum_dx;
    int accum_dy;
} YALinuxMouseThrottleState;

static YALinuxMouseThrottleState g_linux_mouse_throttle = {
    .has_last_emit = false,
    .accum_dx = 0,
    .accum_dy = 0,
};

static const double THROTTLE_INTERVAL_MS = 32.0;

static double timespec_diff_ms(const struct timespec *lhs, const struct timespec *rhs)
{
    const double sec = (double)(lhs->tv_sec - rhs->tv_sec);
    const double nsec = (double)(lhs->tv_nsec - rhs->tv_nsec);
    return sec * 1000.0 + nsec / 1000000.0;
}

void ya_mouse_throttle_reset(void)
{
    g_linux_mouse_throttle.accum_dx = 0;
    g_linux_mouse_throttle.accum_dy = 0;
    g_linux_mouse_throttle.has_last_emit = false;
}

bool ya_mouse_throttle_collect(int dx, int dy, bool force_flush, int *out_dx, int *out_dy)
{
    if (!out_dx || !out_dy)
    {
        return false;
    }

    g_linux_mouse_throttle.accum_dx += dx;
    g_linux_mouse_throttle.accum_dy += dy;

    struct timespec now;
    clock_gettime(CLOCK_MONOTONIC, &now);

    bool should_emit = force_flush;
    if (!should_emit)
    {
        if (!g_linux_mouse_throttle.has_last_emit)
        {
            should_emit = true;
        }
        else
        {
            double elapsed = timespec_diff_ms(&now, &g_linux_mouse_throttle.last_emit);
            if (elapsed >= THROTTLE_INTERVAL_MS)
            {
                should_emit = true;
            }
        }
    }

    if (!should_emit)
    {
        return false;
    }

    if (g_linux_mouse_throttle.accum_dx == 0 && g_linux_mouse_throttle.accum_dy == 0)
    {
        g_linux_mouse_throttle.last_emit = now;
        g_linux_mouse_throttle.has_last_emit = true;
        return false;
    }

    *out_dx = g_linux_mouse_throttle.accum_dx;
    *out_dy = g_linux_mouse_throttle.accum_dy;
    g_linux_mouse_throttle.accum_dx = 0;
    g_linux_mouse_throttle.accum_dy = 0;
    g_linux_mouse_throttle.last_emit = now;
    g_linux_mouse_throttle.has_last_emit = true;
    return true;
}