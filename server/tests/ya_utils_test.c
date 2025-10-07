#include "unity.h"
#include "ya_utils.h"
#include <string.h>
#include <stdlib.h>
#include <unistd.h>
#include <fcntl.h>

void setUp(void)
{
    // 每个测试前的初始化
}

void tearDown(void)
{
    // 每个测试后的清理
}

void test_safe_free_null_pointer(void)
{
    // 测试空指针释放
    void *ptr = NULL;
    safe_free(&ptr);
    TEST_ASSERT_NULL(ptr);
    
    // 测试NULL参数
    safe_free(NULL);
    TEST_ASSERT_TRUE(1); // 应该不会崩溃
}

void test_safe_free_valid_pointer(void)
{
    // 测试有效指针释放
    void *ptr = malloc(100);
    TEST_ASSERT_NOT_NULL(ptr);
    
    safe_free(&ptr);
    TEST_ASSERT_NULL(ptr);
}

void test_safe_free_double_free(void)
{
    // 测试重复释放安全性
    void *ptr = malloc(100);
    TEST_ASSERT_NOT_NULL(ptr);
    
    safe_free(&ptr);
    TEST_ASSERT_NULL(ptr);
    
    // 再次释放应该安全
    safe_free(&ptr);
    TEST_ASSERT_NULL(ptr);
}

void test_get_real_path_limit(void)
{
    // 测试路径长度限制获取
    size_t limit = get_real_path_limit();
    
    // 路径限制应该是合理的值
    TEST_ASSERT_TRUE(limit > 0);
    TEST_ASSERT_TRUE(limit <= 65536); // 合理的上限
    TEST_ASSERT_TRUE(limit >= 256);   // 合理的下限
}

void test_file_exists_nonexistent_file(void)
{
    // 测试不存在的文件
    int result = file_exists("/nonexistent/path/file.txt");
    TEST_ASSERT_FALSE(result);
    
    result = file_exists("");
    TEST_ASSERT_FALSE(result);
}

void test_file_exists_current_directory(void)
{
    // 测试当前目录
    int result = file_exists(".");
    TEST_ASSERT_TRUE(result);
}

void test_file_exists_create_and_test(void)
{
    // 创建一个临时文件进行测试
    const char *test_file = "/tmp/ya_utils_test_file.txt";
    
    // 首先确保文件不存在
    unlink(test_file);
    TEST_ASSERT_FALSE(file_exists(test_file));
    
    // 创建文件
    int fd = open(test_file, O_CREAT | O_WRONLY, 0644);
    if (fd >= 0) {
        close(fd);
        
        // 现在文件应该存在
        TEST_ASSERT_TRUE(file_exists(test_file));
        
        // 清理文件
        unlink(test_file);
    }
}

void test_file_exists_path_normalization(void)
{
    // 测试路径规范化（Windows风格路径）
    // 由于我们在Unix系统上，这主要测试代码不会崩溃
    int result = file_exists("\\tmp\\nonexistent");
    TEST_ASSERT_FALSE(result);
}

void test_to_uint32_basic(void)
{
    // 测试基本的uint32转换
    struct evbuffer *buffer = evbuffer_new();
    
    // 添加一个网络字节序的uint32
    uint32_t test_value = 0x12345678;
    uint32_t network_value = htonl(test_value);
    evbuffer_add(buffer, &network_value, sizeof(network_value));
    
    uint32_t result = to_uint32(buffer, 0);
    TEST_ASSERT_EQUAL(test_value, result);
    
    evbuffer_free(buffer);
}

void test_to_uint32_offset(void)
{
    // 测试带偏移的uint32转换
    struct evbuffer *buffer = evbuffer_new();
    
    // 添加一些前缀数据
    uint32_t prefix = 0xAAAAAAAA;
    evbuffer_add(buffer, &prefix, sizeof(prefix));
    
    // 添加目标数据
    uint32_t test_value = 0x87654321;
    uint32_t network_value = htonl(test_value);
    evbuffer_add(buffer, &network_value, sizeof(network_value));
    
    uint32_t result = to_uint32(buffer, 4); // 从偏移4开始
    TEST_ASSERT_EQUAL(test_value, result);
    
    evbuffer_free(buffer);
}

void test_to_uint32_multiple_values(void)
{
    // 测试缓冲区中的多个uint32值
    struct evbuffer *buffer = evbuffer_new();
    
    uint32_t values[] = {0x11111111, 0x22222222, 0x33333333, 0x44444444};
    int num_values = sizeof(values) / sizeof(values[0]);
    
    // 添加所有值（网络字节序）
    for (int i = 0; i < num_values; i++) {
        uint32_t network_value = htonl(values[i]);
        evbuffer_add(buffer, &network_value, sizeof(network_value));
    }
    
    // 验证每个值
    for (int i = 0; i < num_values; i++) {
        uint32_t result = to_uint32(buffer, i * sizeof(uint32_t));
        TEST_ASSERT_EQUAL(values[i], result);
    }
    
    evbuffer_free(buffer);
}

void test_to_uint32_zero_value(void)
{
    // 测试零值
    struct evbuffer *buffer = evbuffer_new();
    
    uint32_t test_value = 0;
    uint32_t network_value = htonl(test_value);
    evbuffer_add(buffer, &network_value, sizeof(network_value));
    
    uint32_t result = to_uint32(buffer, 0);
    TEST_ASSERT_EQUAL(0, result);
    
    evbuffer_free(buffer);
}

void test_to_uint32_max_value(void)
{
    // 测试最大值
    struct evbuffer *buffer = evbuffer_new();
    
    uint32_t test_value = 0xFFFFFFFF;
    uint32_t network_value = htonl(test_value);
    evbuffer_add(buffer, &network_value, sizeof(network_value));
    
    uint32_t result = to_uint32(buffer, 0);
    TEST_ASSERT_EQUAL(test_value, result);
    
    evbuffer_free(buffer);
}

void test_memory_management_patterns(void)
{
    // 测试常见的内存管理模式
    void *ptrs[10];
    
    // 分配多个指针
    for (int i = 0; i < 10; i++) {
        ptrs[i] = malloc(i * 10 + 1);
        TEST_ASSERT_NOT_NULL(ptrs[i]);
    }
    
    // 释放所有指针
    for (int i = 0; i < 10; i++) {
        safe_free(&ptrs[i]);
        TEST_ASSERT_NULL(ptrs[i]);
    }
    
    // 再次释放应该安全
    for (int i = 0; i < 10; i++) {
        safe_free(&ptrs[i]);
        TEST_ASSERT_NULL(ptrs[i]);
    }
}

int main(void)
{
    UNITY_BEGIN();
    
    RUN_TEST(test_safe_free_null_pointer);
    RUN_TEST(test_safe_free_valid_pointer);
    RUN_TEST(test_safe_free_double_free);
    RUN_TEST(test_get_real_path_limit);
    RUN_TEST(test_file_exists_nonexistent_file);
    RUN_TEST(test_file_exists_current_directory);
    RUN_TEST(test_file_exists_create_and_test);
    RUN_TEST(test_file_exists_path_normalization);
    RUN_TEST(test_to_uint32_basic);
    RUN_TEST(test_to_uint32_offset);
    RUN_TEST(test_to_uint32_multiple_values);
    RUN_TEST(test_to_uint32_zero_value);
    RUN_TEST(test_to_uint32_max_value);
    RUN_TEST(test_memory_management_patterns);
    
    return UNITY_END();
} 