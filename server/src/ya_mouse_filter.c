#include "ya_mouse_filter.h"
#include "ya_logger.h"

#include <math.h>
#include <stdlib.h>
#include <string.h>

#ifndef M_PI
#define M_PI 3.14159265358979323846
#endif

/*
 * 鼠标移动滤波 算法说明 (简化版 - 2024重构)
 *
 * 新架构：客户端负责加速和平滑，服务端只做轻量级处理
 * 
 * 服务端职责：
 *  1) 子像素累积：将浮点位移累积，当累积值 >= 1 像素时输出整数步进
 *  2) 可选的灵敏度调节：pointer_scale 提供全局缩放
 *  3) 算法开关：algorithms_enabled 可关闭所有处理，直接透传
 *
 * 客户端职责 (Flutter TouchpadAccelerator)：
 *  1) 速度计算：基于时间戳的真实速度 (px/s)
 *  2) 动态加速：非线性加速曲线 gain = f(velocity)
 *  3) 低通滤波：平滑输出，减少抖动
 *
 * 优势：
 *  - 延迟更低：客户端高帧率处理，无需等待网络往返
 *  - 体验更好：速度计算更准确，加速曲线更平滑
 *  - 架构清晰：职责分离，便于调试和优化
 */

static inline float clampf(float v, float lo, float hi) {
    return v < lo ? lo : (v > hi ? hi : v);
}

ya_mouse_filter_t *ya_mouse_filter_create(void) {
    ya_mouse_filter_t *ctx = (ya_mouse_filter_t *)calloc(1, sizeof(ya_mouse_filter_t));
    if (!ctx) return NULL;
    // 默认：基线参数
    ctx->pointer_scale = 1.0f; // 默认灵敏度
    ctx->algorithms_enabled = true;
    ctx->vex = 0.0f;    // 速度 EMA x (v2)
    ctx->vey = 0.0f;    // 速度 EMA y (v2)
    ctx->frac_x = 0.0f; // 子像素累积
    ctx->frac_y = 0.0f;
    return ctx;
}

void ya_mouse_filter_destroy(ya_mouse_filter_t *ctx) {
    if (!ctx) return;
    free(ctx);
}

void ya_mouse_filter_reset_state(ya_mouse_filter_t *ctx) {
    if (!ctx) return;
    ctx->vex = 0.0f;    // 重置速度 EMA (v2)
    ctx->vey = 0.0f;
    ctx->frac_x = 0.0f; // 重置子像素累积
    ctx->frac_y = 0.0f;
    YA_LOG_TRACE("[mf] Filter state reset: EMA velocity and subpixel accumulation cleared");
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

    // ===== 旧架构 (Protocol v2): 完整的服务端滤波和加速 =====
    
    // 1. 速度估计（轻量 EMA）
    // 将相对像素视为"帧速度"，用简单 EMA 提高稳定度
    const float alpha_v = 0.3f; // EMA 系数
    ctx->vex = alpha_v * fx + (1.0f - alpha_v) * ctx->vex;
    ctx->vey = alpha_v * fy + (1.0f - alpha_v) * ctx->vey;
    float speed = sqrtf(ctx->vex * ctx->vex + ctx->vey * ctx->vey);

    // 2. 简单加速度（速度相关增益）
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

    // 3. 应用灵敏度和增益
    fx *= (ctx->pointer_scale * gain);
    fy *= (ctx->pointer_scale * gain);

    YA_LOG_TRACE("[mf v2] speed=%.3f gain=%.3f scaled=(%.3f,%.3f)", speed, gain, fx, fy);

    // 4. 子像素累计与量化
    ctx->frac_x += fx;
    ctx->frac_y += fy;
    YA_LOG_TRACE("[mf v2] accum frac=(%.3f,%.3f)", ctx->frac_x, ctx->frac_y);

    int qx = 0, qy = 0;
    if (fabsf(ctx->frac_x) >= 1.0f) {
        qx = (int)(ctx->frac_x > 0.0f ? floorf(ctx->frac_x) : ceilf(ctx->frac_x));
        ctx->frac_x -= (float)qx;
    }
    if (fabsf(ctx->frac_y) >= 1.0f) {
        qy = (int)(ctx->frac_y > 0.0f ? floorf(ctx->frac_y) : ceilf(ctx->frac_y));
        ctx->frac_y -= (float)qy;
    }

    YA_LOG_TRACE("[mf v2] output=(%d,%d) frac-remain=(%.3f,%.3f)", qx, qy, ctx->frac_x, ctx->frac_y);

    if (qx == 0 && qy == 0) {
        *out_count = 0;
        return true;
    }

    steps[0].dx = qx;
    steps[0].dy = qy;
    *out_count = 1;
    return true;
}
