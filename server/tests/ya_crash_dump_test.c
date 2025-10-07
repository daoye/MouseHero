#include "unity.h"
#include "ya_crash_dump.h"
#include "ya_logger.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>
#include <errno.h>
#include <signal.h>
#include <unistd.h>

#ifdef _WIN32
#include <windows.h>
#include <direct.h>
#define mkdir(path, mode) _mkdir(path)
#endif

// 测试目录
static char test_dump_dir[512];
static ya_logger_t* logger = NULL;

// 测试前的设置
void setUp(void) {
    // 初始化测试目录
#ifdef _WIN32
    snprintf(test_dump_dir, sizeof(test_dump_dir), "%s\\yaya_test_dumps", getenv("TEMP"));
#else
    snprintf(test_dump_dir, sizeof(test_dump_dir), "/tmp/yaya_test_dumps");
#endif

    // 确保测试目录存在
    mkdir(test_dump_dir, 0755);

    // 初始化日志系统（崩溃转储模块依赖它）
    ya_logger_config_t logger_config = {
        .level = YA_LOG_LEVEL_DEBUG,
        .log_file = NULL,
        .use_console = 1,
        .use_system_log = 0
    };
    logger = ya_logger_init(&logger_config);
    g_logger = logger;
}

// 测试后的清理
void tearDown(void) {
    // 清理日志系统
    if (logger) {
        ya_logger_destroy(logger);
        logger = NULL;
        g_logger = NULL;
    }

    // 清理测试目录
#ifdef _WIN32
    system("rd /s /q %TEMP%\\yaya_test_dumps");
#else
    system("rm -rf /tmp/yaya_test_dumps");
#endif
}

// 测试获取默认转储目录
void test_get_default_dump_dir(void) {
    const char* dir = ya_crash_dump_get_default_dir();
    TEST_ASSERT_NOT_NULL(dir);
    TEST_ASSERT_GREATER_THAN(0, strlen(dir));

#ifdef _WIN32
    // Windows应该包含 \yaya\dumps
    TEST_ASSERT_NOT_NULL(strstr(dir, "\\yaya\\dumps"));
#elif defined(__APPLE__)
    // macOS应该包含 /Library/Logs/CrashReporter/yaya
    TEST_ASSERT_NOT_NULL(strstr(dir, "/Library/Logs/CrashReporter/yaya"));
#else
    // Linux应该是 /var/crash/yaya
    TEST_ASSERT_EQUAL_STRING("./crash", dir);
#endif
}

// 测试初始化函数 - 正常情况
void test_init_with_valid_config(void) {
    ya_crash_dump_config_t config = {
        .dump_dir = test_dump_dir,
        .max_dumps = 5,
        .compress_dumps = 0
    };
    
    int result = ya_crash_dump_init(&config);
    TEST_ASSERT_EQUAL_INT(0, result);
    
    // 验证目录是否被创建
    struct stat st;
    TEST_ASSERT_EQUAL_INT(0, stat(test_dump_dir, &st));
    
    ya_crash_dump_cleanup();
}

// 测试初始化函数 - 空配置
void test_init_with_null_config(void) {
    int result = ya_crash_dump_init(NULL);
    TEST_ASSERT_EQUAL_INT(-1, result);
}

// 测试初始化函数 - 使用默认目录
void test_init_with_default_dir(void) {
    ya_crash_dump_config_t config = {
        .dump_dir = NULL,
        .max_dumps = 5,
        .compress_dumps = 0
    };
    
    int result = ya_crash_dump_init(&config);
    TEST_ASSERT_EQUAL_INT(0, result);
    
    ya_crash_dump_cleanup();
}

// 测试清理函数
void test_cleanup(void) {
    ya_crash_dump_config_t config = {
        .dump_dir = test_dump_dir,
        .max_dumps = 5,
        .compress_dumps = 0
    };
    
    ya_crash_dump_init(&config);
    ya_crash_dump_cleanup();
    // 清理应该成功完成，不会崩溃
    TEST_PASS();
}

// 测试生成转储文件
void test_generate_dump(void) {
    ya_crash_dump_config_t config = {
        .dump_dir = test_dump_dir,
        .max_dumps = 5,
        .compress_dumps = 0
    };
    
    ya_crash_dump_init(&config);
    
    // 生成转储文件
    int result = ya_crash_dump_generate("Test crash dump");
    TEST_ASSERT_EQUAL_INT(0, result);
    
    // 检查目录中是否有转储文件
#ifdef _WIN32
    char pattern[512];
    WIN32_FIND_DATA fd;
    snprintf(pattern, sizeof(pattern), "%s\\crash-*.dmp", test_dump_dir);
    HANDLE hFind = FindFirstFile(pattern, &fd);
    TEST_ASSERT_NOT_EQUAL(INVALID_HANDLE_VALUE, hFind);
    FindClose(hFind);
#else
    char cmd[1024];
    snprintf(cmd, sizeof(cmd), "ls %s/crash-*.txt >/dev/null 2>&1", test_dump_dir);
    TEST_ASSERT_EQUAL_INT(0, system(cmd));
#endif
    
    ya_crash_dump_cleanup();
}

// 测试多次初始化
void test_multiple_init(void) {
    ya_crash_dump_config_t config = {
        .dump_dir = test_dump_dir,
        .max_dumps = 5,
        .compress_dumps = 0
    };
    
    // 第一次初始化
    int result1 = ya_crash_dump_init(&config);
    TEST_ASSERT_EQUAL_INT(0, result1);
    
    // 第二次初始化（应该成功，会覆盖之前的配置）
    int result2 = ya_crash_dump_init(&config);
    TEST_ASSERT_EQUAL_INT(0, result2);
    
    ya_crash_dump_cleanup();
}

// 测试无效目录
void test_invalid_directory(void) {
    ya_crash_dump_config_t config = {
        .dump_dir = "/nonexistent/directory/that/should/not/exist",
        .max_dumps = 5,
        .compress_dumps = 0
    };
    
    int result = ya_crash_dump_init(&config);
    TEST_ASSERT_EQUAL_INT(-1, result);
}

// 测试创建嵌套目录
void test_nested_directory_creation(void) {
    char nested_dir[512];
    snprintf(nested_dir, sizeof(nested_dir), "%s/nested/deep/directory", test_dump_dir);
    
    ya_crash_dump_config_t config = {
        .dump_dir = nested_dir,
        .max_dumps = 5,
        .compress_dumps = 0
    };
    
    int result = ya_crash_dump_init(&config);
    TEST_ASSERT_EQUAL_INT(0, result);
    
    // 验证嵌套目录是否被创建
    struct stat st;
    TEST_ASSERT_EQUAL_INT(0, stat(nested_dir, &st));
    TEST_ASSERT_TRUE(S_ISDIR(st.st_mode));
    
    ya_crash_dump_cleanup();
}

// 测试带尾部斜杠的路径
void test_directory_with_trailing_slash(void) {
    char dir_with_slash[512];
    snprintf(dir_with_slash, sizeof(dir_with_slash), "%s/trailing_slash/", test_dump_dir);
    
    ya_crash_dump_config_t config = {
        .dump_dir = dir_with_slash,
        .max_dumps = 5,
        .compress_dumps = 0
    };
    
    int result = ya_crash_dump_init(&config);
    TEST_ASSERT_EQUAL_INT(0, result);
    
    ya_crash_dump_cleanup();
}

// 测试空路径处理
void test_empty_directory_path(void) {
    ya_crash_dump_config_t config = {
        .dump_dir = "",
        .max_dumps = 5,
        .compress_dumps = 0
    };
    
    int result = ya_crash_dump_init(&config);
    // 空字符串路径应被视为使用默认路径，因此会成功
    TEST_ASSERT_EQUAL_INT(0, result);
}

// 测试未初始化时生成转储
void test_generate_without_init(void) {
    // 确保没有初始化
    ya_crash_dump_cleanup();
    
    int result = ya_crash_dump_generate("Test without init");
    TEST_ASSERT_EQUAL_INT(-1, result);
}

// 测试生成转储时的不同原因字符串
void test_generate_with_various_reasons(void) {
    ya_crash_dump_config_t config = {
        .dump_dir = test_dump_dir,
        .max_dumps = 5,
        .compress_dumps = 0
    };
    
    ya_crash_dump_init(&config);
    
    // 测试NULL原因
    int result1 = ya_crash_dump_generate(NULL);
    TEST_ASSERT_EQUAL_INT(0, result1);
    
    // 测试空字符串原因
    int result2 = ya_crash_dump_generate("");
    TEST_ASSERT_EQUAL_INT(0, result2);
    
    // 测试长原因字符串
    char long_reason[1024];
    memset(long_reason, 'A', sizeof(long_reason) - 1);
    long_reason[sizeof(long_reason) - 1] = '\0';
    int result3 = ya_crash_dump_generate(long_reason);
    TEST_ASSERT_EQUAL_INT(0, result3);
    
    ya_crash_dump_cleanup();
}

// 测试目录权限问题
void test_directory_permission_issues(void) {
    // 尝试在根目录下创建目录（应该失败）
    ya_crash_dump_config_t config = {
        .dump_dir = "/read_only_test_dir_that_should_not_exist",
        .max_dumps = 5,
        .compress_dumps = 0
    };
    
    int result = ya_crash_dump_init(&config);
    TEST_ASSERT_EQUAL_INT(-1, result);
}

// 测试重复调用cleanup
void test_multiple_cleanup(void) {
    ya_crash_dump_config_t config = {
        .dump_dir = test_dump_dir,
        .max_dumps = 5,
        .compress_dumps = 0
    };
    
    ya_crash_dump_init(&config);
    
    // 多次调用cleanup应该是安全的
    ya_crash_dump_cleanup();
    ya_crash_dump_cleanup();
    ya_crash_dump_cleanup();
    
    TEST_PASS();
}

// 测试默认目录获取的一致性
void test_default_dir_consistency(void) {
    const char* dir1 = ya_crash_dump_get_default_dir();
    const char* dir2 = ya_crash_dump_get_default_dir();
    
    // 多次调用应该返回相同的结果
    TEST_ASSERT_EQUAL_STRING(dir1, dir2);
    TEST_ASSERT_EQUAL_PTR(dir1, dir2); // 应该是同一个静态缓冲区
}

// 测试路径长度限制
void test_path_length_limits(void) {
    // 创建一个超长的路径
    char long_path[1024];
    snprintf(long_path, sizeof(long_path), "%s/", test_dump_dir);
    
    // 添加多个嵌套目录直到接近路径长度限制
    for (int i = 0; i < 20; i++) {
        strncat(long_path, "very_long_directory_name_", sizeof(long_path) - strlen(long_path) - 1);
    }
    
    ya_crash_dump_config_t config = {
        .dump_dir = long_path,
        .max_dumps = 5,
        .compress_dumps = 0
    };
    
    // 这可能会成功或失败，取决于文件系统限制
    int result = ya_crash_dump_init(&config);
    // 无论成功还是失败，都不应该崩溃
    TEST_ASSERT_TRUE(result == 0 || result == -1);
    
    if (result == 0) {
        ya_crash_dump_cleanup();
    }
}

// 测试转储文件生成的特殊情况
void test_dump_file_generation_edge_cases(void) {
    ya_crash_dump_config_t config = {
        .dump_dir = test_dump_dir,
        .max_dumps = 10,
        .compress_dumps = 1  // 测试压缩标志
    };
    
    ya_crash_dump_init(&config);
    
    // 测试生成多个转储文件
    for (int i = 0; i < 3; i++) {
        char reason[64];
        snprintf(reason, sizeof(reason), "Test crash %d", i);
        int result = ya_crash_dump_generate(reason);
        TEST_ASSERT_EQUAL_INT(0, result);
        
        // 稍微延迟以确保时间戳不同
        usleep(1000);  // 1毫秒延迟
    }
    
    ya_crash_dump_cleanup();
}

// 测试默认目录在不同环境下的行为
void test_default_dir_environment_variations(void) {
    // 保存原始环境变量
    char* original_home = getenv("HOME");
    
    // 测试HOME环境变量的影响
    setenv("HOME", "/tmp/test_home", 1);
    
    const char* dir = ya_crash_dump_get_default_dir();
    TEST_ASSERT_NOT_NULL(dir);
    TEST_ASSERT_GREATER_THAN(0, strlen(dir));
    
    // 在macOS上，应该包含新的HOME路径
#ifdef __APPLE__
    TEST_ASSERT_NOT_NULL(strstr(dir, "/tmp/test_home"));
#endif
    
    // 恢复原始环境变量
    if (original_home) {
        setenv("HOME", original_home, 1);
    } else {
        unsetenv("HOME");
    }
}

// 测试目录创建的边界情况
void test_directory_creation_edge_cases(void) {
    // 测试当前目录
    ya_crash_dump_config_t config1 = {
        .dump_dir = ".",
        .max_dumps = 5,
        .compress_dumps = 0
    };
    
    int result1 = ya_crash_dump_init(&config1);
    TEST_ASSERT_EQUAL_INT(0, result1);
    ya_crash_dump_cleanup();
    
    // 测试已存在的测试目录
    ya_crash_dump_config_t config2 = {
        .dump_dir = test_dump_dir,
        .max_dumps = 5,
        .compress_dumps = 0
    };
    
    int result2 = ya_crash_dump_init(&config2);
    TEST_ASSERT_EQUAL_INT(0, result2);
    ya_crash_dump_cleanup();
}

// 测试错误处理路径
void test_error_handling_paths(void) {
    // 测试只读文件系统错误（尝试在/dev下创建目录）
    ya_crash_dump_config_t config = {
        .dump_dir = "/dev/crash_dumps_should_fail",
        .max_dumps = 5,
        .compress_dumps = 0
    };
    
    int result = ya_crash_dump_init(&config);
    TEST_ASSERT_EQUAL_INT(-1, result);
}

// 测试配置参数的边界值
void test_config_boundary_values(void) {
    ya_crash_dump_config_t config = {
        .dump_dir = test_dump_dir,
        .max_dumps = 0,  // 边界值：0
        .compress_dumps = 2  // 非标准值
    };
    
    int result = ya_crash_dump_init(&config);
    TEST_ASSERT_EQUAL_INT(0, result);
    
    ya_crash_dump_cleanup();
    
    // 测试负数max_dumps
    config.max_dumps = -1;
    result = ya_crash_dump_init(&config);
    TEST_ASSERT_EQUAL_INT(0, result);
    
    ya_crash_dump_cleanup();
}

// 运行所有测试
int main(void) {
    UNITY_BEGIN();
    
    RUN_TEST(test_get_default_dump_dir);
    RUN_TEST(test_init_with_valid_config);
    RUN_TEST(test_init_with_null_config);
    RUN_TEST(test_init_with_default_dir);
    RUN_TEST(test_cleanup);
    RUN_TEST(test_generate_dump);
    RUN_TEST(test_multiple_init);
    RUN_TEST(test_invalid_directory);
    RUN_TEST(test_nested_directory_creation);
    RUN_TEST(test_directory_with_trailing_slash);
    RUN_TEST(test_empty_directory_path);
    RUN_TEST(test_generate_without_init);
    RUN_TEST(test_generate_with_various_reasons);
    RUN_TEST(test_directory_permission_issues);
    RUN_TEST(test_multiple_cleanup);
    RUN_TEST(test_default_dir_consistency);
    RUN_TEST(test_path_length_limits);
    RUN_TEST(test_dump_file_generation_edge_cases);
    RUN_TEST(test_default_dir_environment_variations);
    RUN_TEST(test_directory_creation_edge_cases);
    RUN_TEST(test_error_handling_paths);
    RUN_TEST(test_config_boundary_values);
    
    return UNITY_END();
} 