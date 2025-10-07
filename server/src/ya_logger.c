#include "ya_logger.h"
#include <errno.h>
#include <sys/stat.h> 

#ifdef _WIN32
#include <direct.h>
#endif

// 全局日志实例
ya_logger_t* g_logger = NULL;

// 默认日志格式选项
static ya_log_format_t g_log_format = {
    .include_timestamp = 1,
    .include_level = 1,
    #if defined DEBUG
    .include_file_line = 1,
    #else
    .include_file_line = 0,
    #endif
    .date_format = "%Y-%m-%d %H:%M:%S",
    .auto_newline = 1
};

// 缓冲区大小定义
#define MAX_TIMESTAMP_SIZE 32
#define MAX_MESSAGE_SIZE 8192
#define MAX_FINAL_MESSAGE_SIZE 16384
#define TRUNCATE_MARK "... (message truncated)"

// 日志级别名称
static const char* level_names[] = {
    "TRACE", "DEBUG", "INFO", "WARN", "ERROR", "FATAL", "OFF"
};

// 获取日志级别名称
const char* ya_logger_level_name(ya_log_level_t level) {
    if (level >= YA_LOG_LEVEL_TRACE && level <= YA_LOG_LEVEL_OFF) {
        return level_names[level];
    }
    return "UNKNOWN";
}

// 检查文件大小并进行日志轮转
static void check_and_rotate_log(ya_logger_t* logger) {
    if (!logger->config.log_file || !logger->log_fp) {
        return;
    }

    struct stat st;
    if (stat(logger->config.log_file, &st) == 0) {
        if (st.st_size >= logger->config.max_file_size) {
            fclose(logger->log_fp);
            
            // 执行日志文件轮转
            for (int i = logger->config.max_backup_files - 1; i >= 0; i--) {
                char old_name[256], new_name[256];
                if (i == 0) {
                    snprintf(old_name, sizeof(old_name), "%s", logger->config.log_file);
                } else {
                    snprintf(old_name, sizeof(old_name), "%s.%d", logger->config.log_file, i);
                }
                snprintf(new_name, sizeof(new_name), "%s.%d", logger->config.log_file, i + 1);
                rename(old_name, new_name);
            }

            // 重新打开日志文件
            logger->log_fp = fopen(logger->config.log_file, "a");
        }
    }
}

// 格式化时间戳
static void format_timestamp(char* buffer, size_t size) {
    time_t now;
    struct tm tm_now;
    
    time(&now);
#ifdef _WIN32
    localtime_s(&tm_now, &now);
#else
    localtime_r(&now, &tm_now);
#endif
    strftime(buffer, size, g_log_format.date_format, &tm_now);
}

// 写入系统日志
static void write_to_system_log(ya_log_level_t level, const char* message) {
#ifdef _WIN32
    WORD win_level;
    switch (level) {
        case YA_LOG_LEVEL_TRACE:
        case YA_LOG_LEVEL_DEBUG:
            win_level = EVENTLOG_INFORMATION_TYPE;
            break;
        case YA_LOG_LEVEL_WARN:
            win_level = EVENTLOG_WARNING_TYPE;
            break;
        case YA_LOG_LEVEL_ERROR:
        case YA_LOG_LEVEL_FATAL:
            win_level = EVENTLOG_ERROR_TYPE;
            break;
        default:
            win_level = EVENTLOG_INFORMATION_TYPE;
    }
    OutputDebugStringA(message);
#elif defined(__ANDROID__)
    int android_level;
    switch (level) {
        case YA_LOG_LEVEL_TRACE:
            android_level = ANDROID_LOG_VERBOSE;
            break;
        case YA_LOG_LEVEL_DEBUG:
            android_level = ANDROID_LOG_DEBUG;
            break;
        case YA_LOG_LEVEL_INFO:
            android_level = ANDROID_LOG_INFO;
            break;
        case YA_LOG_LEVEL_WARN:
            android_level = ANDROID_LOG_WARN;
            break;
        case YA_LOG_LEVEL_ERROR:
            android_level = ANDROID_LOG_ERROR;
            break;
        case YA_LOG_LEVEL_FATAL:
            android_level = ANDROID_LOG_FATAL;
            break;
        default:
            android_level = ANDROID_LOG_INFO;
    }
    __android_log_write(android_level, "ya_core", message);
#elif defined(__APPLE__)
    os_log_type_t apple_level;
    switch (level) {
        case YA_LOG_LEVEL_TRACE:
        case YA_LOG_LEVEL_DEBUG:
            apple_level = OS_LOG_TYPE_DEBUG;
            break;
        case YA_LOG_LEVEL_INFO:
            apple_level = OS_LOG_TYPE_INFO;
            break;
        case YA_LOG_LEVEL_WARN:
            apple_level = OS_LOG_TYPE_DEFAULT;
            break;
        case YA_LOG_LEVEL_ERROR:
            apple_level = OS_LOG_TYPE_ERROR;
            break;
        case YA_LOG_LEVEL_FATAL:
            apple_level = OS_LOG_TYPE_FAULT;
            break;
        default:
            apple_level = OS_LOG_TYPE_DEFAULT;
    }
    os_log_with_type(OS_LOG_DEFAULT, apple_level, "%{public}s", message);
#elif defined(__linux__)
    int syslog_level;
    switch (level) {
        case YA_LOG_LEVEL_TRACE:
        case YA_LOG_LEVEL_DEBUG:
            syslog_level = LOG_DEBUG;
            break;
        case YA_LOG_LEVEL_INFO:
            syslog_level = LOG_INFO;
            break;
        case YA_LOG_LEVEL_WARN:
            syslog_level = LOG_WARNING;
            break;
        case YA_LOG_LEVEL_ERROR:
            syslog_level = LOG_ERR;
            break;
        case YA_LOG_LEVEL_FATAL:
            syslog_level = LOG_CRIT;
            break;
        default:
            syslog_level = LOG_INFO;
    }
    syslog(syslog_level, "%s", message);
#endif
}

// 初始化日志系统
ya_logger_t* ya_logger_init(const ya_logger_config_t* config) {
    if (!config) {
        return NULL;
    }

    ya_logger_t* logger = (ya_logger_t*)malloc(sizeof(ya_logger_t));
    if (!logger) {
        return NULL;
    }

    memset(logger, 0, sizeof(ya_logger_t));
    logger->config = *config;
    logger->last_error = NULL;

    if (config->log_file) {
        // 确保目录存在
        char* last_slash = strrchr(config->log_file, '/');
        if (last_slash) {
            char dir_path[256] = {0};
            size_t dir_len = last_slash - config->log_file;
            if (dir_len < sizeof(dir_path)) {
                strncpy(dir_path, config->log_file, dir_len);
                #ifdef _WIN32
                _mkdir(dir_path);
                #else
                mkdir(dir_path, 0755);
                #endif
            }
        }

        logger->log_fp = fopen(config->log_file, "a");
        if (!logger->log_fp) {
            free(logger);
            return NULL;
        }
        // 设置无缓冲
        setvbuf(logger->log_fp, NULL, _IONBF, 0);
    }

    // 初始化系统日志
    if (config->use_system_log) {
#ifdef __linux__
        openlog("ya_core", LOG_PID | LOG_CONS, LOG_USER);
#endif
    }

    g_logger = logger;
    return logger;
}

// 销毁日志系统
void ya_logger_destroy(ya_logger_t* logger) {
    if (!logger) {
        return;
    }

    // 如果这是全局logger，先将全局指针设为NULL
    if (g_logger == logger) {
        g_logger = NULL;
    }

    if (logger->log_fp) {
        fclose(logger->log_fp);
        logger->log_fp = NULL;
    }

    if (logger->config.use_system_log) {
#ifdef __linux__
        closelog();
#endif
    }

    free(logger->last_error);
    free(logger);
}

// 设置日志级别
void ya_logger_set_level(ya_logger_t* logger, ya_log_level_t level) {
    if (logger) {
        logger->config.level = level;
    }
}

// 核心日志函数
void ya_logger_log(ya_logger_t* logger, ya_log_level_t level,
                  const char* file, int line, const char* func,
                  const char* format, ...) {
    if (!logger || !format || level == YA_LOG_LEVEL_OFF || level < logger->config.level) {
        return;
    }

    char timestamp[MAX_TIMESTAMP_SIZE] = {0};
    char message[MAX_MESSAGE_SIZE] = {0};
    char final_message[MAX_FINAL_MESSAGE_SIZE] = {0};
    size_t written = 0;
    
    // 格式化时间戳
    if (g_log_format.include_timestamp) {
        format_timestamp(timestamp, sizeof(timestamp));
        written += snprintf(final_message + written, sizeof(final_message) - written, 
                          "[%s] ", timestamp);
    }

    // 添加日志级别
    if (g_log_format.include_level) {
        written += snprintf(final_message + written, sizeof(final_message) - written, 
                          "[%s] ", ya_logger_level_name(level));
    }
    
    // 添加文件和行号
    if (g_log_format.include_file_line && file) {
        written += snprintf(final_message + written, sizeof(final_message) - written, 
                          "[%s:%d] ", file, line);
    }

    // 格式化消息内容
    va_list args;
    va_start(args, format);
    vsnprintf(message, sizeof(message), format, args);
    va_end(args);

    // 添加消息内容
    written += snprintf(final_message + written, sizeof(final_message) - written, 
                       "%s%s", message, g_log_format.auto_newline ? "\n" : "");

    // 写入日志文件
    if (logger->log_fp) {
        check_and_rotate_log(logger);
        if (fputs(final_message, logger->log_fp) == EOF) {
            if (logger->last_error) {
                free(logger->last_error);
            }
            logger->last_error = strdup(strerror(errno));
        }
        fflush(logger->log_fp);
    }

    // 输出到控制台
    if (logger->config.use_console) {
        fputs(final_message, stdout);
        fflush(stdout);
    }

    // 写入系统日志
    if (logger->config.use_system_log) {
        write_to_system_log(level, message);
    }
} 