#pragma once
#include <stdbool.h>
#include <stdint.h>
#include "rs.h"

#ifdef __cplusplus
extern "C" {
#endif

// 根据协议键码(lparam)映射到 CKey。返回 true 表示找到映射。
bool ckey_from_protocol(int32_t code, enum CKey *out);

// 说明：ASCII 字符不再在服务端做基键推断；
// ASCII 注入路径由 `key_ascii_action`/`key_unicode_action` 负责。

#ifdef __cplusplus
}
#endif
