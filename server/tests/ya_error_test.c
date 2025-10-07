#include "ya_error.h"
#include "ya_logger.h"
#include "unity.h"
#include <string.h>
#include <stdlib.h>

// 测试辅助函数的前向声明
static ya_error_code_t test_error_function(void);

// 设置全局logger以便测试
static ya_logger_t* logger = NULL;

void setUp(void) {
    // 初始化日志系统（错误模块依赖它）
    ya_logger_config_t logger_config = {
        .level = YA_LOG_LEVEL_DEBUG,
        .log_file = NULL,
        .use_console = 0,  // 不输出到控制台，避免测试时打印过多日志
        .use_system_log = 0
    };
    logger = ya_logger_init(&logger_config);
    g_logger = logger;
    
    ya_error_init();
}

void tearDown(void) {
    ya_error_cleanup();
    
    // 清理日志系统
    if (logger) {
        ya_logger_destroy(logger);
        logger = NULL;
        g_logger = NULL;
    }
}

// 测试错误设置和获取
void test_error_set_get(void) {
    // 设置错误
    YA_SET_ERROR(YA_ERR_MEMORY, "Failed to allocate memory");
    
    // 验证最后一个错误
    TEST_ASSERT_EQUAL_INT(YA_ERR_MEMORY, ya_error_get_last());
    
    // 验证错误消息
    const char* msg = ya_error_get_message(YA_ERR_MEMORY);
    TEST_ASSERT_NOT_NULL(msg);
    TEST_ASSERT_EQUAL_STRING("Memory allocation failed", msg);
    
    // 验证错误堆栈
    size_t stack_size;
    const ya_error_info_t* stack = ya_error_get_stack(&stack_size);
    TEST_ASSERT_NOT_NULL(stack);
    TEST_ASSERT_EQUAL_INT(1, stack_size);
    TEST_ASSERT_EQUAL_INT(YA_ERR_MEMORY, stack[0].code);
    TEST_ASSERT_EQUAL_STRING("Failed to allocate memory", stack[0].message);
}

// 测试错误堆栈
void test_error_stack(void) {
    // 添加多个错误
    YA_SET_ERROR(YA_ERR_NETWORK, "Network connection failed");
    YA_SET_ERROR(YA_ERR_TIMEOUT, "Operation timed out");
    YA_SET_ERROR(YA_ERR_BUSY, "System is busy");
    
    // 验证错误堆栈
    size_t stack_size;
    const ya_error_info_t* stack = ya_error_get_stack(&stack_size);
    TEST_ASSERT_NOT_NULL(stack);
    TEST_ASSERT_EQUAL_INT(3, stack_size);
    
    // 验证堆栈顺序（最新的错误在最后）
    TEST_ASSERT_EQUAL_INT(YA_ERR_NETWORK, stack[0].code);
    TEST_ASSERT_EQUAL_INT(YA_ERR_TIMEOUT, stack[1].code);
    TEST_ASSERT_EQUAL_INT(YA_ERR_BUSY, stack[2].code);
}

// 测试错误清除
void test_error_clear(void) {
    // 设置一个错误
    YA_SET_ERROR(YA_ERR_NETWORK, "Network error");
    
    // 清除错误
    ya_error_clear();
    
    // 验证错误已清除
    TEST_ASSERT_EQUAL_INT(YA_OK, ya_error_get_last());
    
    size_t stack_size;
    const ya_error_info_t* stack = ya_error_get_stack(&stack_size);
    TEST_ASSERT_EQUAL_INT(0, stack_size);
}

// 测试辅助函数的实现
static ya_error_code_t test_error_function(void) {
    YA_SET_ERROR(YA_ERR_NETWORK, "Network error");
    return YA_ERR_NETWORK;
}

// 测试错误检查宏
void test_error_check_macro(void) {
    ya_error_code_t result = test_error_function();
    TEST_ASSERT_EQUAL_INT(YA_ERR_NETWORK, result);
}

// 测试未初始化时的行为
void test_uninitialized_behavior(void) {
    // 先清理，模拟未初始化状态
    ya_error_cleanup();
    
    // 获取错误应该返回默认值
    ya_error_code_t code = ya_error_get_last();
    TEST_ASSERT_EQUAL_INT(YA_ERR_UNKNOWN, code);
    
    // 获取错误堆栈应该返回NULL
    size_t stack_size;
    const ya_error_info_t* stack = ya_error_get_stack(&stack_size);
    TEST_ASSERT_NULL(stack);
    TEST_ASSERT_EQUAL_INT(0, stack_size);
    
    // 清除错误不应该崩溃
    ya_error_clear();
    
    // 设置错误应该自动初始化
    YA_SET_ERROR(YA_ERR_MEMORY, "Test error");
    TEST_ASSERT_EQUAL_INT(YA_ERR_MEMORY, ya_error_get_last());
}

// 测试所有错误码的消息
void test_all_error_messages(void) {
    ya_error_code_t codes[] = {
        YA_OK,
        YA_ERR_UNKNOWN,
        YA_ERR_MEMORY,
        YA_ERR_TIMEOUT,
        YA_ERR_BUSY,
        YA_ERR_INTERRUPTED,
        YA_ERR_CONFIG_INVALID,
        YA_ERR_CONFIG_NOT_FOUND,
        YA_ERR_CONFIG_TYPE,
        YA_ERR_NETWORK,
        YA_ERR_CONNECTION,
        YA_ERR_SOCKET,
        YA_ERR_PROTOCOL,
        YA_ERR_SESSION_INVALID,
        YA_ERR_SESSION_EXPIRED,
        YA_ERR_SESSION_LIMIT,
        YA_ERR_CMD_INVALID,
        YA_ERR_CMD_PARAMS,
        YA_ERR_CMD_UNSUPPORTED,
        YA_ERR_CMD_FAILED,
        YA_ERR_PERMISSION,
        YA_ERR_AUTH_FAILED,
        YA_ERR_RESOURCE_BUSY,
        YA_ERR_RESOURCE_LIMIT,
        YA_ERR_RESOURCE_NOT_FOUND
    };
    
    size_t num_codes = sizeof(codes) / sizeof(codes[0]);
    
    for (size_t i = 0; i < num_codes; i++) {
        const char* msg = ya_error_get_message(codes[i]);
        TEST_ASSERT_NOT_NULL(msg);
        TEST_ASSERT_GREATER_THAN(0, strlen(msg));
    }
}

// 测试无效错误码
void test_invalid_error_code(void) {
    ya_error_code_t invalid_code = (ya_error_code_t)9999;
    const char* msg = ya_error_get_message(invalid_code);
    TEST_ASSERT_NOT_NULL(msg);
    TEST_ASSERT_EQUAL_STRING("Invalid error code", msg);
}

// 测试错误堆栈溢出
void test_error_stack_overflow(void) {
    // 填满错误堆栈（YA_ERROR_STACK_SIZE = 64）
    for (int i = 0; i < 70; i++) {
        char msg[64];
        snprintf(msg, sizeof(msg), "Error %d", i);
        YA_SET_ERROR(YA_ERR_NETWORK, msg);
    }
    
    // 验证堆栈大小没有超过限制
    size_t stack_size;
    const ya_error_info_t* stack = ya_error_get_stack(&stack_size);
    TEST_ASSERT_NOT_NULL(stack);
    TEST_ASSERT_EQUAL_INT(64, stack_size);  // YA_ERROR_STACK_SIZE
    
    // 验证最新的错误仍然在堆栈中
    TEST_ASSERT_EQUAL_STRING("Error 69", stack[stack_size - 1].message);
    
    // 验证最早的错误已被移除（应该从Error 6开始）
    TEST_ASSERT_EQUAL_STRING("Error 6", stack[0].message);
}

// 测试空参数处理
void test_null_parameters(void) {
    // 测试ya_error_get_stack的NULL参数
    const ya_error_info_t* stack = ya_error_get_stack(NULL);
    TEST_ASSERT_NULL(stack);
    
    // 先清理错误上下文
    ya_error_cleanup();
    
    // 在未初始化状态下测试
    size_t stack_size;
    stack = ya_error_get_stack(&stack_size);
    TEST_ASSERT_NULL(stack);
    TEST_ASSERT_EQUAL_INT(0, stack_size);
}

// 测试错误信息参数处理
void test_error_message_parameters(void) {
    // 测试NULL消息
    ya_error_set(YA_ERR_MEMORY, NULL, __FILE__, __LINE__, __FUNCTION__);
    
    size_t stack_size;
    const ya_error_info_t* stack = ya_error_get_stack(&stack_size);
    TEST_ASSERT_NOT_NULL(stack);
    TEST_ASSERT_EQUAL_INT(1, stack_size);
    TEST_ASSERT_EQUAL_STRING("", stack[0].message);
    
    // 测试NULL文件名
    ya_error_set(YA_ERR_NETWORK, "Test error", NULL, __LINE__, __FUNCTION__);
    stack = ya_error_get_stack(&stack_size);
    TEST_ASSERT_EQUAL_INT(2, stack_size);
    TEST_ASSERT_EQUAL_STRING("", stack[1].file);
    
    // 测试NULL函数名
    ya_error_set(YA_ERR_TIMEOUT, "Test error", __FILE__, __LINE__, NULL);
    stack = ya_error_get_stack(&stack_size);
    TEST_ASSERT_EQUAL_INT(3, stack_size);
    TEST_ASSERT_EQUAL_STRING("", stack[2].function);
}

// 测试长消息截断
void test_long_message_truncation(void) {
    // 创建一个超长的错误消息
    char long_msg[512];
    memset(long_msg, 'A', sizeof(long_msg) - 1);
    long_msg[sizeof(long_msg) - 1] = '\0';
    
    YA_SET_ERROR(YA_ERR_MEMORY, long_msg);
    
    size_t stack_size;
    const ya_error_info_t* stack = ya_error_get_stack(&stack_size);
    TEST_ASSERT_NOT_NULL(stack);
    TEST_ASSERT_EQUAL_INT(1, stack_size);
    
    // 验证消息被正确截断
    TEST_ASSERT_EQUAL_INT(255, strlen(stack[0].message));  // YA_ERROR_MSG_SIZE - 1
}

// 测试长文件名和函数名截断
void test_long_file_function_names(void) {
    // 创建超长的文件名和函数名
    char long_file[300];
    char long_function[200];
    memset(long_file, 'F', sizeof(long_file) - 1);
    long_file[sizeof(long_file) - 1] = '\0';
    memset(long_function, 'G', sizeof(long_function) - 1);
    long_function[sizeof(long_function) - 1] = '\0';
    
    ya_error_set(YA_ERR_NETWORK, "Test", long_file, 100, long_function);
    
    size_t stack_size;
    const ya_error_info_t* stack = ya_error_get_stack(&stack_size);
    TEST_ASSERT_NOT_NULL(stack);
    TEST_ASSERT_EQUAL_INT(1, stack_size);
    
    // 验证文件名被正确截断到255字符 (sizeof(stack[0].file) - 1 = 255)
    TEST_ASSERT_EQUAL_INT(255, strlen(stack[0].file));
    // 验证函数名被正确截断到63字符 (sizeof(stack[0].function) - 1 = 63)
    TEST_ASSERT_EQUAL_INT(63, strlen(stack[0].function));
}

// 测试多次初始化和清理
void test_multiple_init_cleanup(void) {
    // 多次初始化应该是安全的
    ya_error_init();
    ya_error_init();
    ya_error_init();
    
    // 设置一个错误
    YA_SET_ERROR(YA_ERR_MEMORY, "Test error");
    TEST_ASSERT_EQUAL_INT(YA_ERR_MEMORY, ya_error_get_last());
    
    // 多次清理应该是安全的
    ya_error_cleanup();
    ya_error_cleanup();
    ya_error_cleanup();
    
    // 再次初始化应该工作
    ya_error_init();
    YA_SET_ERROR(YA_ERR_NETWORK, "Another test");
    TEST_ASSERT_EQUAL_INT(YA_ERR_NETWORK, ya_error_get_last());
}

// 测试复杂的错误堆栈操作
void test_complex_error_stack_operations(void) {
    // 添加一些错误
    YA_SET_ERROR(YA_ERR_MEMORY, "Memory error 1");
    YA_SET_ERROR(YA_ERR_NETWORK, "Network error 1");
    YA_SET_ERROR(YA_ERR_TIMEOUT, "Timeout error 1");
    
    // 清除错误
    ya_error_clear();
    
    // 验证清除后状态
    TEST_ASSERT_EQUAL_INT(YA_OK, ya_error_get_last());
    size_t stack_size;
    ya_error_get_stack(&stack_size);
    TEST_ASSERT_EQUAL_INT(0, stack_size);
    
    // 再次添加错误
    YA_SET_ERROR(YA_ERR_CONFIG_INVALID, "Config error");
    TEST_ASSERT_EQUAL_INT(YA_ERR_CONFIG_INVALID, ya_error_get_last());
    
    const ya_error_info_t* stack = ya_error_get_stack(&stack_size);
    TEST_ASSERT_EQUAL_INT(1, stack_size);
    TEST_ASSERT_EQUAL_STRING("Config error", stack[0].message);
}

// 测试边界条件
void test_boundary_conditions(void) {
    // 测试行号为0
    ya_error_set(YA_ERR_MEMORY, "Line 0 test", __FILE__, 0, __FUNCTION__);
    
    size_t stack_size;
    const ya_error_info_t* stack = ya_error_get_stack(&stack_size);
    TEST_ASSERT_EQUAL_INT(1, stack_size);
    TEST_ASSERT_EQUAL_INT(0, stack[0].line);
    
    // 测试负行号
    ya_error_set(YA_ERR_NETWORK, "Negative line test", __FILE__, -1, __FUNCTION__);
    stack = ya_error_get_stack(&stack_size);
    TEST_ASSERT_EQUAL_INT(2, stack_size);
    TEST_ASSERT_EQUAL_INT(-1, stack[1].line);
    
    // 测试最大整数行号
    ya_error_set(YA_ERR_TIMEOUT, "Max line test", __FILE__, INT_MAX, __FUNCTION__);
    stack = ya_error_get_stack(&stack_size);
    TEST_ASSERT_EQUAL_INT(3, stack_size);
    TEST_ASSERT_EQUAL_INT(INT_MAX, stack[2].line);
}

// 测试所有系统错误码
void test_system_error_codes(void) {
    ya_error_code_t system_codes[] = {
        YA_ERR_MEMORY,
        YA_ERR_TIMEOUT,
        YA_ERR_BUSY,
        YA_ERR_INTERRUPTED
    };
    
    for (size_t i = 0; i < sizeof(system_codes) / sizeof(system_codes[0]); i++) {
        char msg[64];
        snprintf(msg, sizeof(msg), "System error %zu", i);
        YA_SET_ERROR(system_codes[i], msg);
        
        TEST_ASSERT_EQUAL_INT(system_codes[i], ya_error_get_last());
        
        const char* error_msg = ya_error_get_message(system_codes[i]);
        TEST_ASSERT_NOT_NULL(error_msg);
        TEST_ASSERT_GREATER_THAN(0, strlen(error_msg));
    }
}

// 测试所有配置错误码
void test_config_error_codes(void) {
    ya_error_code_t config_codes[] = {
        YA_ERR_CONFIG_INVALID,
        YA_ERR_CONFIG_NOT_FOUND,
        YA_ERR_CONFIG_TYPE
    };
    
    for (size_t i = 0; i < sizeof(config_codes) / sizeof(config_codes[0]); i++) {
        char msg[64];
        snprintf(msg, sizeof(msg), "Config error %zu", i);
        YA_SET_ERROR(config_codes[i], msg);
        
        TEST_ASSERT_EQUAL_INT(config_codes[i], ya_error_get_last());
    }
}

// 测试所有网络错误码
void test_network_error_codes(void) {
    ya_error_code_t network_codes[] = {
        YA_ERR_NETWORK,
        YA_ERR_CONNECTION,
        YA_ERR_SOCKET,
        YA_ERR_PROTOCOL
    };
    
    for (size_t i = 0; i < sizeof(network_codes) / sizeof(network_codes[0]); i++) {
        char msg[64];
        snprintf(msg, sizeof(msg), "Network error %zu", i);
        YA_SET_ERROR(network_codes[i], msg);
        
        TEST_ASSERT_EQUAL_INT(network_codes[i], ya_error_get_last());
    }
}

int main(void) {
    UNITY_BEGIN();
    
    RUN_TEST(test_error_set_get);
    RUN_TEST(test_error_stack);
    RUN_TEST(test_error_clear);
    RUN_TEST(test_error_check_macro);
    RUN_TEST(test_uninitialized_behavior);
    RUN_TEST(test_all_error_messages);
    RUN_TEST(test_invalid_error_code);
    RUN_TEST(test_error_stack_overflow);
    RUN_TEST(test_null_parameters);
    RUN_TEST(test_error_message_parameters);
    RUN_TEST(test_long_message_truncation);
    RUN_TEST(test_long_file_function_names);
    RUN_TEST(test_multiple_init_cleanup);
    RUN_TEST(test_complex_error_stack_operations);
    RUN_TEST(test_boundary_conditions);
    RUN_TEST(test_system_error_codes);
    RUN_TEST(test_config_error_codes);
    RUN_TEST(test_network_error_codes);
    
    return UNITY_END();
} 