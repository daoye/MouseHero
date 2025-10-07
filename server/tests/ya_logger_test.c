#include "unity.h"
#include "ya_logger.h"
#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <errno.h>
#include <stdlib.h>

// 测试日志文件路径
#define TEST_LOG_FILE "/tmp/test.log"
#define TEST_LOG_CONTENT "test message"

// 测试前的设置
void setUp(void) {
    // 确保全局logger为NULL
    g_logger = NULL;
    
    // 删除可能存在的测试日志文件
    remove(TEST_LOG_FILE);
    remove(TEST_LOG_FILE ".1");
    remove(TEST_LOG_FILE ".2");
    remove(TEST_LOG_FILE ".3");
}

// 测试后的清理
void tearDown(void) {
    // 如果全局logger存在，销毁它
    if (g_logger) {
        ya_logger_destroy(g_logger);
        g_logger = NULL;
    }
    
    remove(TEST_LOG_FILE);
    remove(TEST_LOG_FILE ".1");
    remove(TEST_LOG_FILE ".2");
    remove(TEST_LOG_FILE ".3");
}

// 测试日志初始化的各种情况
void test_logger_init(void) {
    // 测试正常初始化
    ya_logger_config_t config = {
        .level = YA_LOG_LEVEL_DEBUG,
        .log_file = TEST_LOG_FILE,
        .use_console = 0,
        .use_system_log = 0,
        .max_file_size = 1024,
        .max_backup_files = 2
    };

    ya_logger_t* logger = ya_logger_init(&config);
    TEST_ASSERT_NOT_NULL(logger);
    TEST_ASSERT_EQUAL_STRING(TEST_LOG_FILE, logger->config.log_file);
    TEST_ASSERT_EQUAL(YA_LOG_LEVEL_DEBUG, logger->config.level);
    TEST_ASSERT_NOT_NULL(logger->log_fp);
    ya_logger_destroy(logger);

    // 测试无日志文件的初始化
    config.log_file = NULL;
    logger = ya_logger_init(&config);
    TEST_ASSERT_NOT_NULL(logger);
    TEST_ASSERT_NULL(logger->log_fp);
    ya_logger_destroy(logger);

    // 测试只使用控制台输出
    config.use_console = 1;
    logger = ya_logger_init(&config);
    TEST_ASSERT_NOT_NULL(logger);
    ya_logger_destroy(logger);

    // 测试只使用系统日志
    config.use_console = 0;
    config.use_system_log = 1;
    logger = ya_logger_init(&config);
    TEST_ASSERT_NOT_NULL(logger);
    ya_logger_destroy(logger);

    // 测试无效的日志文件路径
    config.log_file = "/dev/null/invalid";
    logger = ya_logger_init(&config);
    TEST_ASSERT_NULL(logger);

    // 测试日志文件权限问题
    config.log_file = TEST_LOG_FILE;
    int fd = open(TEST_LOG_FILE, O_CREAT | O_WRONLY, 0444);
    if (fd >= 0) {
        close(fd);
        chmod(TEST_LOG_FILE, 0444);  // 确保文件是只读的
        logger = ya_logger_init(&config);
        TEST_ASSERT_NULL(logger);
        chmod(TEST_LOG_FILE, 0666);  // 恢复权限以便删除
        remove(TEST_LOG_FILE);
    }
}

// 测试日志级别名称
void test_logger_level_name(void) {
    TEST_ASSERT_EQUAL_STRING("TRACE", ya_logger_level_name(YA_LOG_LEVEL_TRACE));
    TEST_ASSERT_EQUAL_STRING("DEBUG", ya_logger_level_name(YA_LOG_LEVEL_DEBUG));
    TEST_ASSERT_EQUAL_STRING("INFO", ya_logger_level_name(YA_LOG_LEVEL_INFO));
    TEST_ASSERT_EQUAL_STRING("WARN", ya_logger_level_name(YA_LOG_LEVEL_WARN));
    TEST_ASSERT_EQUAL_STRING("ERROR", ya_logger_level_name(YA_LOG_LEVEL_ERROR));
    TEST_ASSERT_EQUAL_STRING("FATAL", ya_logger_level_name(YA_LOG_LEVEL_FATAL));
    TEST_ASSERT_EQUAL_STRING("OFF", ya_logger_level_name(YA_LOG_LEVEL_OFF));
    TEST_ASSERT_EQUAL_STRING("UNKNOWN", ya_logger_level_name(99)); // 无效级别
    TEST_ASSERT_EQUAL_STRING("UNKNOWN", ya_logger_level_name(-1)); // 负数级别
}

// 测试日志写入的各种情况
void test_logger_write(void) {
    // 删除旧的日志文件
    remove(TEST_LOG_FILE);

    ya_logger_config_t config = {
        .level = YA_LOG_LEVEL_DEBUG,
        .log_file = TEST_LOG_FILE,
        .use_console = 1,
        .use_system_log = 0,
        .max_file_size = 1024,
        .max_backup_files = 2
    };

    ya_logger_t* logger = ya_logger_init(&config);
    TEST_ASSERT_NOT_NULL(logger);
    TEST_ASSERT_NOT_NULL(logger->log_fp);
    g_logger = logger;  // 设置全局logger

    // 测试基本日志写入
    YA_LOG_DEBUG("Debug: %s", TEST_LOG_CONTENT);
    YA_LOG_INFO("Info: %s", TEST_LOG_CONTENT);
    YA_LOG_WARN("Warn: %s", TEST_LOG_CONTENT);
    YA_LOG_ERROR("Error: %s", TEST_LOG_CONTENT);
    YA_LOG_FATAL("Fatal: %s", TEST_LOG_CONTENT);

    // 确保日志被写入
    fflush(logger->log_fp);

    // 读取日志文件内容
    FILE* fp = fopen(TEST_LOG_FILE, "r");
    TEST_ASSERT_NOT_NULL(fp);
    
    char buffer[16384] = {0};
    size_t read_size = fread(buffer, 1, sizeof(buffer) - 1, fp);
    TEST_ASSERT_GREATER_THAN(0, read_size);
    buffer[read_size] = '\0';
    fclose(fp);

    // 打印完整的日志内容以便调试
    printf("Log content:\n%s\n", buffer);

    // 验证日志内容
    const char* expected_strings[] = {
        "[DEBUG]",
        "[INFO]",
        "[WARN]",
        "[ERROR]",
        "[FATAL]",
        "Debug: test message",
        "Info: test message",
        "Warn: test message",
        "Error: test message",
        "Fatal: test message"
    };

    for (size_t i = 0; i < sizeof(expected_strings) / sizeof(expected_strings[0]); i++) {
        if (strstr(buffer, expected_strings[i]) == NULL) {
            printf("Failed to find: %s\n", expected_strings[i]);
            TEST_FAIL_MESSAGE("Expected string not found in log output");
        }
    }

    ya_logger_destroy(logger);
}

// 测试日志级别过滤
void test_logger_level_filtering(void) {
    ya_logger_config_t config = {
        .level = YA_LOG_LEVEL_INFO,
        .log_file = TEST_LOG_FILE,
        .use_console = 0,
        .use_system_log = 0,
        .max_file_size = 1024,
        .max_backup_files = 2
    };

    ya_logger_t* logger = ya_logger_init(&config);
    TEST_ASSERT_NOT_NULL(logger);
    g_logger = logger;  // 设置全局logger

    // 测试不同日志级别的过滤
    YA_LOG_TRACE("This should not appear (TRACE)");
    YA_LOG_DEBUG("This should not appear (DEBUG)");
    YA_LOG_INFO("This should appear (INFO)");
    YA_LOG_WARN("This should appear (WARN)");
    YA_LOG_ERROR("This should appear (ERROR)");
    YA_LOG_FATAL("This should appear (FATAL)");

    fflush(logger->log_fp);

    // 读取日志文件内容
    FILE* fp = fopen(TEST_LOG_FILE, "r");
    TEST_ASSERT_NOT_NULL(fp);
    
    char buffer[4096] = {0};
    fread(buffer, 1, sizeof(buffer) - 1, fp);
    fclose(fp);

    // 验证日志内容
    TEST_ASSERT_NULL(strstr(buffer, "This should not appear"));
    TEST_ASSERT_NOT_NULL(strstr(buffer, "This should appear"));

    // 测试动态更改日志级别
    ya_logger_set_level(logger, YA_LOG_LEVEL_TRACE);
    YA_LOG_TRACE("This should now appear (TRACE)");
    
    fflush(logger->log_fp);
    
    fp = fopen(TEST_LOG_FILE, "r");
    TEST_ASSERT_NOT_NULL(fp);
    memset(buffer, 0, sizeof(buffer));
    fread(buffer, 1, sizeof(buffer) - 1, fp);
    fclose(fp);
    
    TEST_ASSERT_NOT_NULL(strstr(buffer, "This should now appear (TRACE)"));

    // 测试OFF级别
    ya_logger_set_level(logger, YA_LOG_LEVEL_OFF);
    YA_LOG_FATAL("This should not appear (OFF)");
    
    fflush(logger->log_fp);
    
    // 获取当前文件大小
    struct stat st;
    TEST_ASSERT_EQUAL(0, stat(TEST_LOG_FILE, &st));
    long size_before = st.st_size;

    // 写入更多日志
    YA_LOG_FATAL("This should also not appear (OFF)");
    fflush(logger->log_fp);

    // 验证文件大小没有变化
    TEST_ASSERT_EQUAL(0, stat(TEST_LOG_FILE, &st));
    TEST_ASSERT_EQUAL(size_before, st.st_size);

    ya_logger_destroy(logger);
}

// 测试日志文件轮转
void test_logger_rotation(void) {
    ya_logger_config_t config = {
        .level = YA_LOG_LEVEL_DEBUG,
        .log_file = TEST_LOG_FILE,
        .use_console = 0,
        .use_system_log = 0,
        .max_file_size = 200,
        .max_backup_files = 3
    };

    ya_logger_t* logger = ya_logger_init(&config);
    TEST_ASSERT_NOT_NULL(logger);
    g_logger = logger;  // 设置全局logger

    // 写入足够多的日志触发多次轮转
    for (int i = 0; i < 30; i++) {
        YA_LOG_INFO("Test log rotation message %d with some padding to make it longer...", i);
        fflush(logger->log_fp);
    }

    // 确保日志被写入
    fflush(logger->log_fp);

    // 验证所有备份文件是否存在
    struct stat st;
    TEST_ASSERT_EQUAL(0, stat(TEST_LOG_FILE, &st));
    TEST_ASSERT_EQUAL(0, stat(TEST_LOG_FILE ".1", &st));
    TEST_ASSERT_EQUAL(0, stat(TEST_LOG_FILE ".2", &st));
    TEST_ASSERT_EQUAL(0, stat(TEST_LOG_FILE ".3", &st));

    // 验证最新的日志文件大小是否合理
    TEST_ASSERT_GREATER_OR_EQUAL(0, st.st_size);
    TEST_ASSERT_LESS_OR_EQUAL(config.max_file_size * 2, st.st_size);

    // 测试轮转时的错误处理
    // 创建一个只读的备份文件
    int fd = open(TEST_LOG_FILE ".1", O_CREAT | O_WRONLY, 0444);
    if (fd >= 0) {
        close(fd);
        // 尝试写入更多日志
        for (int i = 0; i < 10; i++) {
            YA_LOG_INFO("Additional log message %d", i);
        }
        // 清理
        chmod(TEST_LOG_FILE ".1", 0666);
    }
}

// 测试条件日志
void test_conditional_logging(void) {
    ya_logger_config_t config = {
        .level = YA_LOG_LEVEL_DEBUG,
        .log_file = TEST_LOG_FILE,
        .use_console = 0,
        .use_system_log = 0,
        .max_file_size = 1024,
        .max_backup_files = 2
    };

    ya_logger_t* logger = ya_logger_init(&config);
    TEST_ASSERT_NOT_NULL(logger);
    g_logger = logger;  // 设置全局logger

    // 测试各种条件
    int value = 100;
    YA_LOG_IF(YA_LOG_LEVEL_INFO, value > 50, "Condition met: %d", value);
    YA_LOG_IF(YA_LOG_LEVEL_INFO, value < 50, "Condition not met: %d", value);
    YA_LOG_IF(YA_LOG_LEVEL_INFO, value == 100, "Exact match: %d", value);
    YA_LOG_IF(YA_LOG_LEVEL_INFO, value != 100, "Should not appear");
    YA_LOG_IF(YA_LOG_LEVEL_TRACE, value > 50, "Should not appear due to level");

    // 测试复杂条件
    YA_LOG_IF(YA_LOG_LEVEL_INFO, value > 0 && value < 200, "Complex condition 1");
    YA_LOG_IF(YA_LOG_LEVEL_INFO, value < 0 || value > 200, "Complex condition 2");
    YA_LOG_IF(YA_LOG_LEVEL_INFO, !(value < 100), "Complex condition 3");

    // 测试边界条件
    YA_LOG_IF(YA_LOG_LEVEL_INFO, 1, "Always true");
    YA_LOG_IF(YA_LOG_LEVEL_INFO, 0, "Always false");
    YA_LOG_IF(YA_LOG_LEVEL_INFO, value != 0, "Non-zero value");

    fflush(logger->log_fp);

    // 读取日志文件内容
    FILE* fp = fopen(TEST_LOG_FILE, "r");
    TEST_ASSERT_NOT_NULL(fp);
    
    char buffer[4096] = {0};
    fread(buffer, 1, sizeof(buffer) - 1, fp);
    fclose(fp);

    // 验证日志内容
    TEST_ASSERT_NOT_NULL(strstr(buffer, "Condition met: 100"));
    TEST_ASSERT_NOT_NULL(strstr(buffer, "Exact match: 100"));
    TEST_ASSERT_NOT_NULL(strstr(buffer, "Complex condition 1"));
    TEST_ASSERT_NOT_NULL(strstr(buffer, "Complex condition 3"));
    TEST_ASSERT_NOT_NULL(strstr(buffer, "Always true"));
    TEST_ASSERT_NOT_NULL(strstr(buffer, "Non-zero value"));
    TEST_ASSERT_NULL(strstr(buffer, "Condition not met"));
    TEST_ASSERT_NULL(strstr(buffer, "Should not appear"));
    TEST_ASSERT_NULL(strstr(buffer, "Complex condition 2"));
    TEST_ASSERT_NULL(strstr(buffer, "Always false"));
}

// 测试错误处理
void test_error_handling(void) {
    // 测试无效的配置
    ya_logger_config_t config = {
        .level = YA_LOG_LEVEL_DEBUG,
        .log_file = "/invalid/path/test.log",
        .use_console = 0,
        .use_system_log = 0,
        .max_file_size = 1024,
        .max_backup_files = 2
    };

    ya_logger_t* logger = ya_logger_init(&config);
    TEST_ASSERT_NULL(logger);

    // 测试NULL logger
    ya_logger_destroy(NULL);

    // 测试各种边界情况
    config.log_file = TEST_LOG_FILE;
    logger = ya_logger_init(&config);
    TEST_ASSERT_NOT_NULL(logger);
    g_logger = logger;  // 设置全局logger
    
    // 测试无效的日志级别
    ya_logger_set_level(logger, -1);
    ya_logger_set_level(logger, 99);
    ya_logger_set_level(NULL, YA_LOG_LEVEL_INFO);

    // 测试格式化错误
    YA_LOG_INFO("%d", "not a number");  // 格式不匹配
    YA_LOG_INFO("%s", NULL);  // NULL字符串
    YA_LOG_INFO(NULL);  // NULL格式字符串
    YA_LOG_INFO("");    // 空字符串

    // 测试文件操作错误
    if (logger->log_fp) {
        FILE* old_fp = logger->log_fp;
        logger->log_fp = NULL;
        YA_LOG_INFO("This should handle NULL file pointer");
        logger->log_fp = old_fp;  // 恢复文件指针
    }

    ya_logger_destroy(logger);

    // 测试文件权限问题
    int fd = open(TEST_LOG_FILE, O_CREAT | O_WRONLY, 0444);
    if (fd >= 0) {
        close(fd);
        chmod(TEST_LOG_FILE, 0444);  // 确保文件是只读的
        logger = ya_logger_init(&config);
        TEST_ASSERT_NULL(logger);
        chmod(TEST_LOG_FILE, 0666);  // 恢复权限以便删除
        remove(TEST_LOG_FILE);
    }
}

// 测试系统日志
void test_system_logging(void) {
    ya_logger_config_t config = {
        .level = YA_LOG_LEVEL_DEBUG,
        .log_file = TEST_LOG_FILE,
        .use_console = 1,
        .use_system_log = 1,
        .max_file_size = 1024,
        .max_backup_files = 2
    };

    ya_logger_t* logger = ya_logger_init(&config);
    TEST_ASSERT_NOT_NULL(logger);
    g_logger = logger;  // 设置全局logger

    // 测试所有日志级别
    YA_LOG_TRACE("Trace message");
    YA_LOG_DEBUG("Debug message");
    YA_LOG_INFO("Info message");
    YA_LOG_WARN("Warning message");
    YA_LOG_ERROR("Error message");
    YA_LOG_FATAL("Fatal message");

    // 测试特殊格式
    YA_LOG_INFO("Format test: %d %s %.2f", 42, "string", 3.14);
    YA_LOG_INFO("Multiple lines\ntest\nmessage");
    YA_LOG_INFO("Special chars: \t\r\n");
    YA_LOG_INFO("Unicode: 你好，世界！");
    YA_LOG_INFO("Emoji: 😀 🌍 ⭐");

    // 测试长消息
    char long_message[1024];
    memset(long_message, 'A', sizeof(long_message) - 1);
    long_message[sizeof(long_message) - 1] = '\0';
    YA_LOG_INFO("Long message: %s", long_message);

    TEST_PASS();
}

// 测试日志格式化
void test_logger_formatting(void) {
    // 删除旧的日志文件
    remove(TEST_LOG_FILE);

    ya_logger_config_t config = {
        .level = YA_LOG_LEVEL_DEBUG,
        .log_file = TEST_LOG_FILE,
        .use_console = 0,
        .use_system_log = 0,
        .max_file_size = 1024,
        .max_backup_files = 2
    };

    ya_logger_t* logger = ya_logger_init(&config);
    TEST_ASSERT_NOT_NULL(logger);
    TEST_ASSERT_NOT_NULL(logger->log_fp);
    g_logger = logger;  // 设置全局logger

    // 测试基本数据类型
    YA_LOG_INFO("Integer: %d", 42);
    YA_LOG_INFO("String: %s", "test");
    YA_LOG_INFO("Float: %.2f", 3.14159);
    YA_LOG_INFO("Multiple: %d %s %.2f", 42, "test", 3.14159);
    YA_LOG_INFO("Hex: 0x%x", 255);
    YA_LOG_INFO("Char: %c", 'A');

    // 确保日志被写入
    fflush(logger->log_fp);

    // 读取日志文件内容
    FILE* fp = fopen(TEST_LOG_FILE, "r");
    TEST_ASSERT_NOT_NULL(fp);
    
    char buffer[16384] = {0};
    size_t read_size = fread(buffer, 1, sizeof(buffer) - 1, fp);
    TEST_ASSERT_GREATER_THAN(0, read_size);
    buffer[read_size] = '\0';
    fclose(fp);

    // 打印完整的日志内容以便调试
    printf("Log content:\n%s\n", buffer);

    // 验证格式化输出
    const char* expected_strings[] = {
        "Integer: 42",
        "String: test",
        "Float: 3.14",
        "Multiple: 42 test 3.14",
        "Hex: 0xff",
        "Char: A"
    };

    for (size_t i = 0; i < sizeof(expected_strings) / sizeof(expected_strings[0]); i++) {
        if (strstr(buffer, expected_strings[i]) == NULL) {
            printf("Failed to find: %s\n", expected_strings[i]);
            printf("Log content:\n%s\n", buffer);  // 打印完整的日志内容以便调试
            TEST_FAIL_MESSAGE("Expected string not found in log output");
        }
    }

    ya_logger_destroy(logger);
}

// 运行所有测试
int main(void) {
    UNITY_BEGIN();
    
    RUN_TEST(test_logger_init);
    RUN_TEST(test_logger_level_name);
    RUN_TEST(test_logger_write);
    RUN_TEST(test_logger_level_filtering);
    RUN_TEST(test_logger_rotation);
    RUN_TEST(test_conditional_logging);
    RUN_TEST(test_error_handling);
    RUN_TEST(test_system_logging);
    RUN_TEST(test_logger_formatting);
    
    return UNITY_END();
} 