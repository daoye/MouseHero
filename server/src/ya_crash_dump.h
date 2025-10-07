#ifndef YA_CRASH_DUMP_H
#define YA_CRASH_DUMP_H

#include <stddef.h>

// 崩溃转储配置
typedef struct {
    const char* dump_dir;      // 转储文件保存目录
    size_t max_dumps;          // 最大保留的转储文件数量
    int compress_dumps;        // 是否压缩转储文件
} ya_crash_dump_config_t;

// 初始化崩溃转储系统
int ya_crash_dump_init(const ya_crash_dump_config_t* config);

// 清理崩溃转储系统
void ya_crash_dump_cleanup(void);

// 获取默认的转储目录
const char* ya_crash_dump_get_default_dir(void);

// 手动生成转储文件
int ya_crash_dump_generate(const char* reason);

#endif // YA_CRASH_DUMP_H 