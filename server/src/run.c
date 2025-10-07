#include "ya_utils.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#ifndef _WIN32
#include <execinfo.h>
#include <pwd.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/stat.h>
#else
#include <direct.h>
#include <stdlib.h>

char* realpath(const char* path, char* resolved_path) {
    char* result;
    if (resolved_path == NULL) {
        result = _fullpath(NULL, path, _MAX_PATH);
    } else {
        result = _fullpath(resolved_path, path, _MAX_PATH);
    }
    return result;
}
#endif
#include <signal.h>
#include <unistd.h>
#include <errno.h>

#include "ya_config.h"
#include "ya_logger.h"
#include "ya_server.h"
#include "ya_crash_dump.h"

YA_ServerContext svr_context = {0};
YA_Config config = {0};

// 获取默认日志路径
static const char* get_default_log_path() {
    static char log_path[512] = {0};
    
#ifdef _WIN32
    // Windows: 当前应用所在路径下 server.log
    char exe_path[512] = {0};
    char* last_separator = NULL;
    GetModuleFileNameA(NULL, exe_path, sizeof(exe_path));
    last_separator = strrchr(exe_path, '\\');
    if (last_separator) {
        *(last_separator + 1) = '\0';
        snprintf(log_path, sizeof(log_path), "%sserver.log", exe_path);
    } else {
        snprintf(log_path, sizeof(log_path), "server.log");
    }
#elif defined(__APPLE__)
    // macOS: ~/Library/Logs/yaya/server.log
    const char* home = getenv("HOME");
    if (!home) {
        struct passwd* pwd = getpwuid(getuid());
        if (pwd) {
            home = pwd->pw_dir;
        }
    }
    if (home) {
        snprintf(log_path, sizeof(log_path), "%s/Library/Logs/yaya/server.log", home);
    } else {
        snprintf(log_path, sizeof(log_path), "/var/log/yaya/server.log");
    }
#else
    // Linux: /var/log/yaya/server.log
    snprintf(log_path, sizeof(log_path), "/var/log/yaya/server.log");
#endif

    return log_path;
}

// 确保日志目录存在
static int ensure_log_directory(const char* log_file) {
    if (!log_file || !*log_file) return 0;
    const char* last_slash = strrchr(log_file, '/');
    const char* last_backslash = strrchr(log_file, '\\');
    const char* last_separator = (last_slash > last_backslash) ? last_slash : last_backslash;
    if (!last_separator) {
        // 没有分隔符，说明日志文件就在当前目录，无需创建目录
        return 0;
    }
    size_t dir_len = last_separator - log_file;
    if (dir_len == 0 || dir_len >= 512) return 0;
    char dir_path[512] = {0};
    strncpy(dir_path, log_file, dir_len);
    dir_path[dir_len] = '\0';
    return ya_ensure_dir(dir_path);
}

void sigsegv_handler(int sig)
{
    YA_LOG_ERROR("Received signal %d, generating crash dump...", sig);
    ya_crash_dump_generate("Segmentation fault");
    exit(1);
}

// 初始化日志系统
static int init_logger(const char* config_file) {
    const char* log_level_str = ya_config_get(&config, "logger", "level");
    const char* log_file = ya_config_get(&config, "logger", "file");
    const char* use_console_str = ya_config_get(&config, "logger", "use_console");
    const char* use_system_log_str = ya_config_get(&config, "logger", "use_system_log");
    const char* max_file_size_str = ya_config_get(&config, "logger", "max_file_size");
    const char* max_backup_files_str = ya_config_get(&config, "logger", "max_backup_files");

    // 设置默认值
    ya_logger_config_t logger_config = {
        .level = YA_LOG_LEVEL_INFO,
        // .log_file = get_default_log_path(),
        .use_console = 1,
        .use_system_log = 1,
        .max_file_size = 10 * 1024 * 1024,  // 10MB
        .max_backup_files = 5
    };

    // 解析配置值
    if (log_level_str) {
        if (strcmp(log_level_str, "TRACE") == 0) logger_config.level = YA_LOG_LEVEL_TRACE;
        else if (strcmp(log_level_str, "DEBUG") == 0) logger_config.level = YA_LOG_LEVEL_DEBUG;
        else if (strcmp(log_level_str, "INFO") == 0) logger_config.level = YA_LOG_LEVEL_INFO;
        else if (strcmp(log_level_str, "WARN") == 0) logger_config.level = YA_LOG_LEVEL_WARN;
        else if (strcmp(log_level_str, "ERROR") == 0) logger_config.level = YA_LOG_LEVEL_ERROR;
        else if (strcmp(log_level_str, "FATAL") == 0) logger_config.level = YA_LOG_LEVEL_FATAL;
        else if (strcmp(log_level_str, "OFF") == 0) logger_config.level = YA_LOG_LEVEL_OFF;
    }

    if (log_file) {
        logger_config.log_file = log_file;
    }

    // 确保日志目录存在
    if (ensure_log_directory(logger_config.log_file) != 0 && errno != EEXIST) {
        fprintf(stderr, "Failed to create log directory for: %s\n", logger_config.log_file);
        return -1;
    }

    if (use_console_str) {
        logger_config.use_console = atoi(use_console_str);
    }

    if (use_system_log_str) {
        logger_config.use_system_log = atoi(use_system_log_str);
    }

    if (max_file_size_str) {
        logger_config.max_file_size = atol(max_file_size_str);
    }

    if (max_backup_files_str) {
        logger_config.max_backup_files = atoi(max_backup_files_str);
    }

    // 初始化日志系统
    g_logger = ya_logger_init(&logger_config);
    if (!g_logger) {
        fprintf(stderr, "Failed to initialize logger\n");
        return -1;
    }

    return 0;
}

int main(int argc, char *argv[])
{
    #if defined DEBUG
    signal(SIGSEGV, sigsegv_handler);  
    #endif

    ya_logger_config_t bootstrap_logger_config = {
        .level = YA_LOG_LEVEL_INFO,
        .log_file = NULL,
        .use_console = 1,
        .use_system_log = 0,
        .max_file_size = 0,
        .max_backup_files = 0
    };
    g_logger = ya_logger_init(&bootstrap_logger_config);
    if (!g_logger) {
        fprintf(stderr, "Failed to initialize bootstrap logger\n");
    }

    char* config_file;

    if (argc > 1)
    {
        config_file = realpath(argv[1], NULL);
    }
    else
    {
        char exe_path[1024];
        if (ya_get_executable_path(exe_path, sizeof(exe_path)) == 0) {
            char* exe_dir = ya_get_dir_path(exe_path);
            if (exe_dir) {
                char path[1024];
                snprintf(path, sizeof(path), "%s/server.conf", exe_dir);
                config_file = realpath(path, NULL);
            }
        }
    }

    if (!config_file || file_exists(config_file) == 0)
    {
        YA_LOG_FATAL("Failed to resolve the path for the configuration file.");
        exit(1);
    }

    svr_context.config_file_path = config_file;

    ya_config_init(&config);

    if (ya_config_parse(config_file, &config) != 0)
    {
        YA_LOG_FATAL("Failed to parse the configuration file '%s'.", config_file);
        exit(1);
    }

    #ifdef DEBUG
    ya_crash_dump_config_t dump_config = {
        .dump_dir = NULL,                               // 使用默认目录
        .max_dumps = 10,                                // 保留最近10个转储文件
        .compress_dumps = 1                             // 压缩转储文件
    };
    
    if (ya_crash_dump_init(&dump_config) != 0) {
        YA_LOG_FATAL("Failed to initialize crash dump system.");
        exit(1);
    }
    #endif

    if (g_logger) {
        ya_logger_destroy(g_logger);
        g_logger = NULL;
    }
    
    if (init_logger(config_file) != 0) {
        exit(1);
    }

    YA_LOG_INFO("Server starting with config file: %s", config_file);

    svr_context.session_addr = ya_config_get(&config, "session", "listener");
    if (!svr_context.session_addr || !*svr_context.session_addr)
    {
        YA_LOG_FATAL("Failed to resolve the session listen address.");
        exit(1);
    }
    int addr_len = sizeof(svr_context.session_sock_addr);
    if (evutil_parse_sockaddr_port(svr_context.session_addr, (struct sockaddr *)&svr_context.session_sock_addr,
                                   &addr_len) < 0)
    {
        YA_LOG_ERROR("Failed to parse the session listen address '%s'.", svr_context.session_addr);
        exit(1);
    }

    svr_context.command_addr = ya_config_get(&config, "command", "listener");
    if (!svr_context.command_addr || !*svr_context.command_addr)
    {
        YA_LOG_FATAL("Failed to resolve the command listen address.");
        exit(1);
    }
    
    if (evutil_parse_sockaddr_port(svr_context.command_addr, (struct sockaddr *)&svr_context.command_sock_addr,
                                   &addr_len) < 0)
    {
        YA_LOG_ERROR("Failed to parse the command listen address '%s'.", svr_context.command_addr);
        exit(1);
    }

    svr_context.http_addr = ya_config_get(&config, "http", "listener");
    if (!svr_context.http_addr || !*svr_context.http_addr) {
        YA_LOG_FATAL("Failed to resolve the http listen address.");
        exit(1);
    }
    int http_addr_len = sizeof(svr_context.http_sock_addr);
    if (evutil_parse_sockaddr_port(svr_context.http_addr, (struct sockaddr *)&svr_context.http_sock_addr, &http_addr_len) < 0) {
        YA_LOG_ERROR("Failed to parse the http listen address '%s'.", svr_context.http_addr);
        exit(1);
    }

    YA_LOG_DEBUG("Starting server...");

    int c = start_server();

    ya_logger_destroy(g_logger);
    #ifdef DEBUG
    ya_crash_dump_cleanup();
    #endif
    safe_free((void **)&config_file);
    ya_config_free(&config);

    return c;
}
