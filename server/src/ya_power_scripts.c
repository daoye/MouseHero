#include <limits.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#if !defined(_WIN32) && !defined(_WIN64)
#  include <sys/stat.h>
#  include <unistd.h>
#endif
#if defined(__APPLE__)
#  include <mach-o/dyld.h>
#endif
#if defined(_WIN32) || defined(_WIN64)
#  include <windows.h>
#endif

#include "ya_power_scripts.h"
#include "ya_utils.h"

// 解析可执行文件所在目录（用于定位 scripts 目录）
static void ya_get_exe_dir(char *out_dir, size_t out_size)
{
#if defined(_WIN32) || defined(_WIN64)
    DWORD len = GetModuleFileNameA(NULL, out_dir, (DWORD)out_size);
    if (len > 0 && len < out_size)
    {
        // 去掉文件名，仅保留目录
        for (int i = (int)len - 1; i >= 0; --i)
        {
            if (out_dir[i] == '\\' || out_dir[i] == '/')
            {
                out_dir[i] = '\0';
                break;
            }
        }
    }
    else
    {
        strncpy(out_dir, ".", out_size);
        out_dir[out_size - 1] = '\0';
    }
#elif defined(__APPLE__)
    uint32_t size = (uint32_t)out_size;
    if (_NSGetExecutablePath(out_dir, &size) == 0)
    {
        // 去掉文件名
        size_t len = strnlen(out_dir, out_size);
        for (int i = (int)len - 1; i >= 0; --i)
        {
            if (out_dir[i] == '/')
            {
                out_dir[i] = '\0';
                break;
            }
        }
    }
    else
    {
        strncpy(out_dir, ".", out_size);
        out_dir[out_size - 1] = '\0';
    }
#else
    // Linux: 读取 /proc/self/exe
    ssize_t n = readlink("/proc/self/exe", out_dir, out_size - 1);
    if (n > 0)
    {
        out_dir[n] = '\0';
        size_t len = (size_t)n;
        for (int i = (int)len - 1; i >= 0; --i)
        {
            if (out_dir[i] == '/')
            {
                out_dir[i] = '\0';
                break;
            }
        }
    }
    else
    {
        strncpy(out_dir, ".", out_size);
        out_dir[out_size - 1] = '\0';
    }
#endif
}

int ya_run_power_script(const char *script_basename)
{
    char exe_dir[PATH_MAX];
    ya_get_exe_dir(exe_dir, sizeof(exe_dir));

    char script_path[PATH_MAX];
#if defined(_WIN32) || defined(_WIN64)
    // Windows: 优先 scripts\\<name>.bat，其次 scripts\\windows\\<name>.bat（兼容旧结构）
    char cmd[PATH_MAX * 2];
    int rc = -1;
    // Debug 模式优先从源码目录 server/scripts 运行
    #ifdef DEBUG
    snprintf(script_path, sizeof(script_path), "%s\\..\\..\\..\\..\\scripts\\%s.bat", exe_dir, script_basename);
    snprintf(cmd, sizeof(cmd), "cmd /c \"\"%s\"\"", script_path);
    rc = system(cmd);
    #endif
    if (rc != 0)
    {
        // 回退到打包后的 scripts 目录
        snprintf(script_path, sizeof(script_path), "%s\\scripts\\%s.bat", exe_dir, script_basename);
        snprintf(cmd, sizeof(cmd), "cmd /c \"\"%s\"\"", script_path);
        rc = system(cmd);
    }
    if (rc != 0)
    {
        snprintf(script_path, sizeof(script_path), "%s\\scripts\\windows\\%s.bat", exe_dir, script_basename);
        snprintf(cmd, sizeof(cmd), "cmd /c \"\"%s\"\"", script_path);
        rc = system(cmd);
    }
    return rc;
#elif defined(__APPLE__)
    struct stat st;
    char cmd[PATH_MAX * 2];
    #ifdef DEBUG
    snprintf(script_path, sizeof(script_path), "%s/../../../../scripts/%s.sh", exe_dir, script_basename);
    if (stat(script_path, &st) == 0)
    {
        snprintf(cmd, sizeof(cmd), "/bin/sh \"%s\"", script_path);
        int rc = system(cmd);
        if (rc == 0)
            return rc;
    }
    #endif
    // 优先从用户目录 ~/Library/MouseHero/scripts
    char home_dir[PATH_MAX];
    if (ya_get_home_dir(home_dir, sizeof(home_dir)) == 0)
    {
        // ~/Library/MouseHero/scripts/<name>.sh
        snprintf(script_path, sizeof(script_path), "%s/Library/MouseHero/scripts/%s.sh", home_dir, script_basename);
    }

    if (stat(script_path, &st) != 0)
    {
        // 回退到打包后的 app 资源目录：Contents/Resources/scripts/<name>.sh
        snprintf(script_path, sizeof(script_path), "%s/../Resources/scripts/%s.sh", exe_dir, script_basename);
    }
    snprintf(cmd, sizeof(cmd), "/bin/sh \"%s\"", script_path);
    return system(cmd);
#else
    // Linux: 优先 scripts/<name>.sh，其次 scripts/linux/<name>.sh（兼容旧结构）
    struct stat st;
    char cmd[PATH_MAX * 2];
    // Debug 模式优先从源码目录 server/scripts 运行
    #ifdef DEBUG
    snprintf(script_path, sizeof(script_path), "%s/../../../../scripts/%s.sh", exe_dir, script_basename);
    if (stat(script_path, &st) == 0)
    {
        snprintf(cmd, sizeof(cmd), "/bin/sh \"%s\"", script_path);
        int rc = system(cmd);
        if (rc == 0)
            return rc;
    }
    #endif
    // 回退到打包后的 scripts 目录
    snprintf(script_path, sizeof(script_path), "%s/scripts/%s.sh", exe_dir, script_basename);
    if (stat(script_path, &st) != 0)
    {
        snprintf(script_path, sizeof(script_path), "%s/scripts/linux/%s.sh", exe_dir, script_basename);
    }
    snprintf(cmd, sizeof(cmd), "/bin/sh \"%s\"", script_path);
    return system(cmd);
#endif
}
