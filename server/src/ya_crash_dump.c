#include "ya_crash_dump.h"
#include "ya_logger.h"
#include "ya_utils.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <signal.h>

#ifdef _WIN32
#include <winsock2.h> 
#include <windows.h>
#include <dbghelp.h>
#include <direct.h>
#elif defined(__APPLE__)
#include <sys/types.h>
#include <unistd.h>
#include <execinfo.h>
#include <pwd.h>
#else
#include <sys/stat.h>
#include <sys/types.h>
#include <unistd.h>
#include <execinfo.h>
#include <sys/resource.h>
#endif

static ya_crash_dump_config_t g_config = {0};

// 创建目录（包括父目录）
static int create_directory_recursive(const char* path) {
    int ret = ya_ensure_dir(path);
    if (ret != 0) {
        YA_LOG_ERROR("Failed to create directory %s: %s", path, strerror(errno));
        return -1;
    }
    return 0;
}

// 获取默认的转储目录
const char* ya_crash_dump_get_default_dir(void) {
    static char dump_dir[512] = {0};
    
#ifdef _WIN32
    // Windows: %LOCALAPPDATA%\yaya\dumps
    char* local_app_data = getenv("LOCALAPPDATA");
    if (local_app_data) {
        snprintf(dump_dir, sizeof(dump_dir), "%s\\yaya\\dumps", local_app_data);
    }
#elif defined(__APPLE__)
    // macOS: ~/Library/Logs/CrashReporter/yaya
    const char* home = getenv("HOME");
    if (home) {
        snprintf(dump_dir, sizeof(dump_dir), "%s/Library/Logs/CrashReporter/yaya", home);
    }
#else
    // Linux: /var/crash/yaya
    snprintf(dump_dir, sizeof(dump_dir), "./crash");
#endif
    
    return dump_dir;
}

// 初始化崩溃转储系统
int ya_crash_dump_init(const ya_crash_dump_config_t* config) {
    if (!config) {
        YA_LOG_ERROR("Invalid configuration");
        return -1;
    }
    
    // 使用默认目录或配置的目录
    const char* dump_dir = config->dump_dir ? config->dump_dir : ya_crash_dump_get_default_dir();
    
    // 创建转储目录（包括父目录）
    if (create_directory_recursive(dump_dir) != 0) {
        YA_LOG_ERROR("Failed to create dump directory: %s", dump_dir);
        return -1;
    }
    
    // 保存配置
    memcpy(&g_config, config, sizeof(ya_crash_dump_config_t));
    if (!g_config.dump_dir) {
        g_config.dump_dir = ya_crash_dump_get_default_dir();
    }
    
    YA_LOG_DEBUG("Crash dump system initialized, dump directory: %s", dump_dir);
    return 0;
}

// 清理崩溃转储系统
void ya_crash_dump_cleanup(void) {
    memset(&g_config, 0, sizeof(g_config));
}

// 生成崩溃转储文件
static void generate_crash_dump(int sig, const char* reason) {
    char dump_file[512];
    time_t now;
    struct tm* tm_info;
    
    time(&now);
    tm_info = localtime(&now);
    
#ifdef _WIN32
    snprintf(dump_file, sizeof(dump_file), "%s\\crash-%04d%02d%02d-%02d%02d%02d.dmp",
             g_config.dump_dir,
             tm_info->tm_year + 1900, tm_info->tm_mon + 1, tm_info->tm_mday,
             tm_info->tm_hour, tm_info->tm_min, tm_info->tm_sec);
#else
    snprintf(dump_file, sizeof(dump_file), "%s/crash-%04d%02d%02d-%02d%02d%02d.txt",
             g_config.dump_dir,
             tm_info->tm_year + 1900, tm_info->tm_mon + 1, tm_info->tm_mday,
             tm_info->tm_hour, tm_info->tm_min, tm_info->tm_sec);
#endif
    
    FILE* fp = fopen(dump_file, "w");
    if (!fp) {
        YA_LOG_ERROR("Failed to create dump file %s: %s", dump_file, strerror(errno));
        return;
    }
    
    fprintf(fp, "Crash Report\n");
    fprintf(fp, "============\n\n");
    fprintf(fp, "Time: %s", ctime(&now));
    fprintf(fp, "Signal: %d\n", sig);
    if (reason) {
        fprintf(fp, "Reason: %s\n", reason);
    }
    fprintf(fp, "\nStack Trace:\n");
    
#ifdef _WIN32
    // Windows: 使用 MiniDumpWriteDump
    HANDLE process = GetCurrentProcess();
    HANDLE file = CreateFileA(dump_file, GENERIC_WRITE, 0, NULL, CREATE_ALWAYS,
                            FILE_ATTRIBUTE_NORMAL, NULL);
    
    if (file != INVALID_HANDLE_VALUE) {
        MINIDUMP_EXCEPTION_INFORMATION mei;
        mei.ThreadId = GetCurrentThreadId();
        mei.ExceptionPointers = NULL;
        mei.ClientPointers = FALSE;
        
        MiniDumpWriteDump(process, GetCurrentProcessId(), file,
                         MiniDumpNormal, &mei, NULL, NULL);
        CloseHandle(file);
    }
#else
    // Linux/macOS: 使用 backtrace
    void* array[50];
    size_t size = backtrace(array, 50);
    char** strings = backtrace_symbols(array, size);
    
    if (strings) {
        for (size_t i = 0; i < size; i++) {
            fprintf(fp, "%s\n", strings[i]);
        }
        free(strings);
    }
#endif
    
    fclose(fp);
}

// 崩溃信号处理函数
static void crash_handler(int sig) {
    // 生成崩溃转储
    generate_crash_dump(sig, "Program crashed");
    
#ifdef _WIN32
    raise(sig);
#else
    struct sigaction sa;
    memset(&sa, 0, sizeof(sa));
    sa.sa_handler = SIG_DFL;
    sigemptyset(&sa.sa_mask);
    sa.sa_flags = 0;
    
    sigaction(sig, &sa, NULL);
    raise(sig);
#endif
}

// 手动生成转储文件
int ya_crash_dump_generate(const char* reason) {
    if (!g_config.dump_dir) {
        YA_LOG_ERROR("Crash dump system not initialized");
        return -1;
    }
    
#ifdef _WIN32
    // Windows 平台直接生成转储文件，不设置信号处理
#else
    // Unix 平台设置信号处理程序
    struct sigaction sa;
    memset(&sa, 0, sizeof(sa));
    sa.sa_handler = crash_handler;
    sigemptyset(&sa.sa_mask);
    sa.sa_flags = 0;
    
    sigaction(SIGSEGV, &sa, NULL);
    sigaction(SIGABRT, &sa, NULL);
    sigaction(SIGFPE, &sa, NULL);
    sigaction(SIGILL, &sa, NULL);
    sigaction(SIGBUS, &sa, NULL);
#endif
    
    // 生成转储文件
    generate_crash_dump(0, reason);
    return 0;
} 