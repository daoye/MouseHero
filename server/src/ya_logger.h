#pragma once

#include <stdarg.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>

#ifdef _WIN32
#include <windows.h>
#elif defined(__ANDROID__)
#include <android/log.h>
#elif defined(__linux__)
#include <syslog.h>
#elif defined(__APPLE__)
#include <os/log.h>
#endif

// 日志级别定义
typedef enum {
    YA_LOG_LEVEL_TRACE = 0,
    YA_LOG_LEVEL_DEBUG = 1,
    YA_LOG_LEVEL_INFO = 2,
    YA_LOG_LEVEL_WARN = 3,
    YA_LOG_LEVEL_ERROR = 4,
    YA_LOG_LEVEL_FATAL = 5,
    YA_LOG_LEVEL_OFF = 6
} ya_log_level_t;

// 日志配置结构
typedef struct {
    ya_log_level_t level;          // 当前日志级别
    const char* log_file;          // 日志文件路径
    int use_console;               // 是否输出到控制台
    int use_system_log;            // 是否使用系统日志
    size_t max_file_size;          // 单个日志文件最大大小(字节)
    int max_backup_files;          // 最大备份文件数
} ya_logger_config_t;

// 日志上下文结构
typedef struct {
    ya_logger_config_t config;
    FILE* log_fp;
    char* last_error;
} ya_logger_t;

// 日志格式化选项
typedef struct {
    int include_timestamp;     // 是否包含时间戳
    int include_level;         // 是否包含日志级别
    int include_file_line;     // 是否包含文件和行号
    const char* date_format;   // 时间戳格式
    int auto_newline;         // 是否自动添加换行符
} ya_log_format_t;

// 全局日志实例
extern ya_logger_t* g_logger;

// 初始化日志系统
ya_logger_t* ya_logger_init(const ya_logger_config_t* config);

// 销毁日志系统
void ya_logger_destroy(ya_logger_t* logger);

// 设置日志级别
void ya_logger_set_level(ya_logger_t* logger, ya_log_level_t level);

// 获取日志级别名称
const char* ya_logger_level_name(ya_log_level_t level);

// 核心日志函数
void ya_logger_log(ya_logger_t* logger, ya_log_level_t level, 
                  const char* file, int line, const char* func,
                  const char* format, ...);

// 日志宏定义
#define YA_LOG_TRACE(format, ...) \
    ya_logger_log(g_logger, YA_LOG_LEVEL_TRACE, __FILE__, __LINE__, __func__, format, ##__VA_ARGS__)

#define YA_LOG_DEBUG(format, ...) \
    ya_logger_log(g_logger, YA_LOG_LEVEL_DEBUG, __FILE__, __LINE__, __func__, format, ##__VA_ARGS__)

#define YA_LOG_INFO(format, ...) \
    ya_logger_log(g_logger, YA_LOG_LEVEL_INFO, __FILE__, __LINE__, __func__, format, ##__VA_ARGS__)

#define YA_LOG_WARN(format, ...) \
    ya_logger_log(g_logger, YA_LOG_LEVEL_WARN, __FILE__, __LINE__, __func__, format, ##__VA_ARGS__)

#define YA_LOG_ERROR(format, ...) \
    ya_logger_log(g_logger, YA_LOG_LEVEL_ERROR, __FILE__, __LINE__, __func__, format, ##__VA_ARGS__)

#define YA_LOG_FATAL(format, ...) \
    ya_logger_log(g_logger, YA_LOG_LEVEL_FATAL, __FILE__, __LINE__, __func__, format, ##__VA_ARGS__)

// 条件日志宏
#define YA_LOG_IF(level, condition, format, ...) \
    do { \
        if (condition) { \
            ya_logger_log(g_logger, level, __FILE__, __LINE__, __func__, format, ##__VA_ARGS__); \
        } \
    } while(0)
