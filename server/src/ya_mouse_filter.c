#include "ya_mouse_filter.h"
#include "ya_logger.h"

#include <math.h>
#include <stdlib.h>
#include <string.h>

#ifndef M_PI
#define M_PI 3.14159265358979323846
#endif

/*
 * 鼠标移动滤波 算法说明
 *
 * 
 *  1) 速度估计（EMA）：用轻量速度 EMA 稳定瞬时速度估计。
 *     v_ema[t] = alpha_v * v_in[t] + (1 - alpha_v) * v_ema[t-1]
 *     其中 v_in[t] 取本帧相对位移（像素）作为速度近似。
 *
 *  2) 简单加速度（速度相关增益）：
 *     gain = base * (1 + k * max(|v_ema|-v0, 0)^p), 然后限制到 accel_max。
 *     直觉：低速≈1，高速适度放大，跨屏不吃力；参数均可由客户端后续下发覆盖。
 *
 *  3) DPI/灵敏度 + 子像素累计：
 *     f = (dx,dy) * pointer_scale * gain
 *     累计到 (frac_x, frac_y)，当 |frac| >= 1 时取整输出一次步子，并回收整数部分：
 *       qx = floor(frac_x) 或 ceil(frac_x)（按正负分别取地板/天花板以减少偏置）
 *       frac_x -= qx，y 同理。
 *     每次最多输出 1 步（与 handler 的注入循环兼容）；若量化后为 (0,0)，则不输出。
 *
 * 参数影响：
 *  - pointer_scale：线性敏感度，整体快慢首选参数。
 *  - alpha_v：速度 EMA 系数，越大越灵敏（抖动可能增），越小越稳（响应略慢）。
 *  - accel_k / v0 / p / accel_max：控制速度相关增益的爬升速度、起点、形状与上限。
 *
 * 平台差异：
 *  - 算法仅输出相对步子，不涉屏幕坐标系；若平台注入层 Y 方向与直觉相反，应在注入层做一次翻转。
 */
struct ya_mouse_filter {
    float pointer_scale;  // 指针全局缩放（灵敏度）
    bool  algorithms_enabled; // 算法总开关（false 时透传输入）

    // 状态
    float vex;  // 速度 EMA x
    float vey;  // 速度 EMA y
    float frac_x; // 子像素累计 x
    float frac_y; // 子像素累计 y
};

static inline float clampf(float v, float lo, float hi) {
    return v < lo ? lo : (v > hi ? hi : v);
}

ya_mouse_filter_t *ya_mouse_filter_create(void) {
    ya_mouse_filter_t *ctx = (ya_mouse_filter_t *)calloc(1, sizeof(ya_mouse_filter_t));
    if (!ctx) return NULL;
    // 默认：基线参数
    ctx->pointer_scale = 1.0f; // DPI
    ctx->algorithms_enabled = true;
    ctx->vex = 0.0f;
    ctx->vey = 0.0f;
    ctx->frac_x = 0.0f;
    ctx->frac_y = 0.0f;
    return ctx;
}

void ya_mouse_filter_destroy(ya_mouse_filter_t *ctx) {
    if (!ctx) return;
    free(ctx);
}

void ya_mouse_filter_set_curve_enabled(ya_mouse_filter_t *ctx, bool enabled) { (void)ctx; (void)enabled; }

void ya_mouse_filter_set_ema_alpha(ya_mouse_filter_t *ctx, float alpha) { (void)ctx; (void)alpha; }

void ya_mouse_filter_set_limits(ya_mouse_filter_t *ctx, float max_speed, float max_accel) { (void)ctx; (void)max_speed; (void)max_accel; }

void ya_mouse_filter_set_adaptive_params(ya_mouse_filter_t *ctx,
                                         float alpha_min, float alpha_max, float s_ref_alpha,
                                         float g_min, float g_max, float s_ref_gain,
                                         float beta,
                                         float lerp_step_len,
                                         float s_bezier, float d_bezier) {
    (void)ctx; (void)alpha_min; (void)alpha_max; (void)s_ref_alpha;
    (void)g_min; (void)g_max; (void)s_ref_gain; (void)beta;
    (void)lerp_step_len; (void)s_bezier; (void)d_bezier;
}

void ya_mouse_filter_set_pointer_scale(ya_mouse_filter_t *ctx, float sensitivity) {
    if (!ctx) return;
    ctx->pointer_scale = fmaxf(0.1f, sensitivity);
}

void ya_mouse_filter_set_ballistic_params(ya_mouse_filter_t *ctx,
                                          float gain_exp,
                                          float s_alpha_cutoff,
                                          float s_flick,
                                          float flick_boost) {
    (void)ctx; (void)gain_exp; (void)s_alpha_cutoff; (void)s_flick; (void)flick_boost;
}

void ya_mouse_filter_set_low_latency_thresholds(ya_mouse_filter_t *ctx,
                                                float s_low_latency,
                                                float d_low_latency) {
    (void)ctx; (void)s_low_latency; (void)d_low_latency;
}

void ya_mouse_filter_set_algorithms_enabled(ya_mouse_filter_t *ctx, bool enabled) {
    if (!ctx) return;
    ctx->algorithms_enabled = enabled;
}

bool ya_mouse_filter_process(ya_mouse_filter_t *ctx,
                             int in_dx, int in_dy,
                             ya_mouse_step_t *steps,
                             size_t steps_cap,
                             size_t *out_count) {
    if (!ctx || !steps || steps_cap == 0 || !out_count) return false;

    // 输入（像素）：handler 已对传输单位做还原与取整
    float fx = (float)in_dx;
    float fy = (float)in_dy;

    // 总开关：关闭则直接透传
    if (!ctx->algorithms_enabled) {
        if (in_dx == 0 && in_dy == 0) { *out_count = 0; return true; }
        steps[0].dx = in_dx;
        steps[0].dy = in_dy;
        *out_count = 1;
        return true;
    }

    // 速度估计（轻量 EMA）
    // 将相对像素视为“帧速度”，用简单 EMA 提高稳定度
    const float alpha_v = 0.3f; // 默认值；下个版本由客户端参数覆盖
    ctx->vex = alpha_v * fx + (1.0f - alpha_v) * ctx->vex;
    ctx->vey = alpha_v * fy + (1.0f - alpha_v) * ctx->vey;
    float speed = sqrtf(ctx->vex * ctx->vex + ctx->vey * ctx->vey);

    // 简单加速度（透明、可调、限幅）
    // gain = base * (1 + k * max(speed - v0, 0)^p), clamp to accel_max
    const float base_gain = 1.0f;
    const float accel_k   = 0.02f;
    const float v0        = 2.0f;
    const float p         = 1.0f;
    const float accel_max = 3.0f;
    float dv = speed - v0;
    if (dv < 0.0f) dv = 0.0f;
    float gain = base_gain * (1.0f + accel_k * powf(dv, p));
    if (gain > accel_max) gain = accel_max;

    // DPI/灵敏度（pointer_scale 来自客户端参数或默认值）
    fx *= (ctx->pointer_scale * gain);
    fy *= (ctx->pointer_scale * gain);

    YA_LOG_TRACE("[mf] speed=%.3f gain=%.3f after-gain dxdy=(%.3f,%.3f)", speed, gain, fx, fy);

    // 子像素累计与量化
    ctx->frac_x += fx;
    ctx->frac_y += fy;
    YA_LOG_TRACE("[mf] accum pre-quant frac=(%.3f,%.3f)", ctx->frac_x, ctx->frac_y);

    int qx = 0, qy = 0;
    if (fabsf(ctx->frac_x) >= 1.0f) {
        qx = (int)(ctx->frac_x > 0.0f ? floorf(ctx->frac_x) : ceilf(ctx->frac_x));
        ctx->frac_x -= (float)qx;
    }
    if (fabsf(ctx->frac_y) >= 1.0f) {
        qy = (int)(ctx->frac_y > 0.0f ? floorf(ctx->frac_y) : ceilf(ctx->frac_y));
        ctx->frac_y -= (float)qy;
    }

    YA_LOG_TRACE("[mf] quant q=(%d,%d) frac-after=(%.3f,%.3f)", qx, qy, ctx->frac_x, ctx->frac_y);

    if (qx == 0 && qy == 0) {
        *out_count = 0;
        return true;
    }

    steps[0].dx = qx;
    steps[0].dy = qy;
    *out_count = 1;
    return true;
}
