#pragma once
#include <stdint.h>
#include <stdbool.h>
#include "rs.h"

#ifdef __cplusplus
extern "C" {
#endif

// 注入可打印 ASCII 字符：
// - Windows：将“最终字符”还原为物理基键 + Shift（仅必要时），否则回退到 Unicode 注入
// - 非 Windows：直接以 Unicode 注入最终字符
YAError ya_inject_ascii(int32_t code, enum CDirection dir);

#ifdef __cplusplus
}
#endif
