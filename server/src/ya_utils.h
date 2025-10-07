#pragma once
#include "event2/buffer.h"

#include <stdint.h>
#include <stdlib.h>
#include <event2/util.h>
#include <stdbool.h>

void safe_free(void **ptr);

size_t get_real_path_limit();

int file_exists(const char *path);

uint32_t to_uint32(struct evbuffer *buffer, int start);

int ya_ensure_dir(const char* path);

void ya_get_peer_addr(evutil_socket_t fd, char *addr_buf, size_t buf_len);

void ya_normalize_path(char* path);

int ya_get_executable_path(char* path_buf, size_t buf_len);

char* ya_get_dir_path(char* file_path);

char* ya_get_macs_csv(void);

// 获取主显示器的逻辑缩放因子（display scale）。
// 优先读取环境变量 YA_DISPLAY_SCALE (>0 有效)，否则：
// - macOS: 使用 CoreGraphics 的像素/点比例
// - Windows: 使用系统 DPI（LOGPIXELSX）/ 96.0
// - Linux: Wayland/Xorg：尝试 GDK_SCALE、GDK_DPI_SCALE、QT_SCALE_FACTOR；否则 1.0
// 返回值范围一般在 [0.5, 4.0]，若不可得则返回 1.0
float ya_primary_scale(void);

// 获取当前用户 Home 目录，成功返回 0，失败返回 -1。
// out_home 以 NUL 结尾，长度为 out_size。
int ya_get_home_dir(char *out_home, size_t out_size);

bool ya_is_wayland_session(void);
