#pragma once

#include <stddef.h>
#include <stdbool.h>
#include <stdint.h>

// 前置声明，避免循环依赖
struct ya_mouse_filter;
typedef struct ya_mouse_filter ya_mouse_filter_t;

typedef struct {
    int dx;
    int dy;
} ya_mouse_step_t;

// 创建/销毁每客户端滤波上下文
ya_mouse_filter_t *ya_mouse_filter_create(void);
void ya_mouse_filter_destroy(ya_mouse_filter_t *ctx);

// 运行时配置（可按客户端切换）
void ya_mouse_filter_set_curve_enabled(ya_mouse_filter_t *ctx, bool enabled);
void ya_mouse_filter_set_ema_alpha(ya_mouse_filter_t *ctx, float alpha);
void ya_mouse_filter_set_limits(ya_mouse_filter_t *ctx, float max_speed, float max_accel);

// 自适应参数配置（可选）：
// - EMA 自适应区间与参考速度
// - 速度自适应增益参数
// - 预测强度 beta
// - LERP 步长（像素）
// - 曲线插值触发门限（速度/距离）
void ya_mouse_filter_set_adaptive_params(ya_mouse_filter_t *ctx,
                                         float alpha_min, float alpha_max, float s_ref_alpha,
                                         float g_min, float g_max, float s_ref_gain,
                                         float beta,
                                         float lerp_step_len,
                                         float s_bezier, float d_bezier);

// 指针基础灵敏度（整体尺度放大），适配小屏手势到大屏像素
void ya_mouse_filter_set_pointer_scale(ya_mouse_filter_t *ctx, float sensitivity);

// 弹道曲线参数：
// - gain_exp: 非线性指数（>1 更强调高速增益）
// - s_alpha_cutoff: 高速直接关闭 EMA 平滑的速度阈值
// - s_flick: 触发快速滑动增强的速度阈值
// - flick_boost: 快速滑动的额外倍增
void ya_mouse_filter_set_ballistic_params(ya_mouse_filter_t *ctx,
                                          float gain_exp,
                                          float s_alpha_cutoff,
                                          float s_flick,
                                          float flick_boost);

// 低延迟模式阈值：当速度或本段距离超过阈值时，跳过平滑/插值，直接输出单步
void ya_mouse_filter_set_low_latency_thresholds(ya_mouse_filter_t *ctx,
                                                float s_low_latency,
                                                float d_low_latency);

// 总开关：启用/关闭全部滤波与插值算法；关闭时直接透传输入位移
void ya_mouse_filter_set_algorithms_enabled(ya_mouse_filter_t *ctx, bool enabled);

// 处理一次来自客户端的相对移动输入，输出若干步平滑后的微步
// 输入: dx, dy 为相对位移（与原实现一致，单位: 像素）
// 输出: steps 用于承载微步，最多写入 steps_cap 个；实际写入数量置于 *out_count
// 返回: true 表示有输出，false 表示无输出或参数错误
bool ya_mouse_filter_process(ya_mouse_filter_t *ctx,
                             int dx, int dy,
                             ya_mouse_step_t *steps,
                             size_t steps_cap,
                             size_t *out_count);
