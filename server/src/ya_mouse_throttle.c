#include "ya_mouse_throttle.h"
#include "ya_logger.h"

#include <time.h>

typedef struct
{
    struct timespec last_emit;
    bool has_last_emit;
    int accum_dx;
    int accum_dy;
} YAMouseThrottleState;

static YAMouseThrottleState g_mouse_throttle = {
    .has_last_emit = false,
    .accum_dx = 0,
    .accum_dy = 0,
};

// 优化后的节流间隔：客户端已做平滑，服务端可以更激进地减少延迟
static const double THROTTLE_INTERVAL_LOW_SPEED_MS = 4.0;   // 低速：4ms (250Hz) - 更低延迟
static const double THROTTLE_INTERVAL_HIGH_SPEED_MS = 8.0;  // 高速：8ms (125Hz) - 大幅减少延迟
static const int LOW_SPEED_THRESHOLD = 3; // 累积位移阈值（像素）- 更敏感

static double timespec_diff_ms(const struct timespec *lhs, const struct timespec *rhs)
{
    const double sec = (double)(lhs->tv_sec - rhs->tv_sec);
    const double nsec = (double)(lhs->tv_nsec - rhs->tv_nsec);
    return sec * 1000.0 + nsec / 1000000.0;
}

// 计算当前累积的移动速度（像素）
static int get_accumulated_distance(void)
{
    int dx = g_mouse_throttle.accum_dx;
    int dy = g_mouse_throttle.accum_dy;
    // 使用曼哈顿距离作为速度估计（避免sqrt）
    return (dx < 0 ? -dx : dx) + (dy < 0 ? -dy : dy);
}

void ya_mouse_throttle_reset(void)
{
    g_mouse_throttle.accum_dx = 0;
    g_mouse_throttle.accum_dy = 0;
    g_mouse_throttle.has_last_emit = false;
}

bool ya_mouse_throttle_collect(int dx, int dy, bool force_flush, int *out_dx, int *out_dy)
{
    if (!out_dx || !out_dy)
    {
        return false;
    }

    g_mouse_throttle.accum_dx += dx;
    g_mouse_throttle.accum_dy += dy;

    struct timespec now;
    clock_gettime(CLOCK_MONOTONIC, &now);

    bool should_emit = force_flush;
    if (!should_emit)
    {
        if (!g_mouse_throttle.has_last_emit)
        {
            should_emit = true;
        }
        else
        {
            double elapsed = timespec_diff_ms(&now, &g_mouse_throttle.last_emit);
            // 自适应节流：根据累积移动距离选择间隔
            int distance = get_accumulated_distance();
            double threshold = (distance < LOW_SPEED_THRESHOLD) 
                ? THROTTLE_INTERVAL_LOW_SPEED_MS 
                : THROTTLE_INTERVAL_HIGH_SPEED_MS;
            
            YA_LOG_TRACE("[throttle] distance=%d, threshold=%.1fms, elapsed=%.1fms", 
                         distance, threshold, elapsed);
            
            if (elapsed >= threshold)
            {
                should_emit = true;
            }
        }
    }

    if (!should_emit)
    {
        return false;
    }

    if (g_mouse_throttle.accum_dx == 0 && g_mouse_throttle.accum_dy == 0)
    {
        g_mouse_throttle.last_emit = now;
        g_mouse_throttle.has_last_emit = true;
        return false;
    }

    *out_dx = g_mouse_throttle.accum_dx;
    *out_dy = g_mouse_throttle.accum_dy;
    g_mouse_throttle.accum_dx = 0;
    g_mouse_throttle.accum_dy = 0;
    g_mouse_throttle.last_emit = now;
    g_mouse_throttle.has_last_emit = true;
    return true;
}

bool ya_mouse_throttle_flush(int *out_dx, int *out_dy)
{
    if (!out_dx || !out_dy)
    {
        return false;
    }

    if (g_mouse_throttle.accum_dx == 0 && g_mouse_throttle.accum_dy == 0)
    {
        return false;
    }

    *out_dx = g_mouse_throttle.accum_dx;
    *out_dy = g_mouse_throttle.accum_dy;
    g_mouse_throttle.accum_dx = 0;
    g_mouse_throttle.accum_dy = 0;
    
    struct timespec now;
    clock_gettime(CLOCK_MONOTONIC, &now);
    g_mouse_throttle.last_emit = now;
    g_mouse_throttle.has_last_emit = true;
    
    return true;
}