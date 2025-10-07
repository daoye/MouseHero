#include "unity.h"
#include "ya_authorize.h"
#include "ya_event.h"
#include <string.h>
#include <stdlib.h>

void setUp(void)
{
    // 每个测试前的初始化
}

void tearDown(void)
{
    // 每个测试后的清理
}

void test_authorize_with_null_param(void)
{
    // 测试空参数
    bool result = authorize(NULL);
    
    TEST_ASSERT_FALSE(result);
    TEST_ASSERT_NOT_NULL(get_authorize_error());
    TEST_ASSERT_EQUAL_STRING("Unknown error", get_authorize_error());
}

void test_authorize_with_low_version(void)
{
    // 测试版本过低
    YAAuthorizeEventRequest request;
    request.version = 0;
    request.type = CLIENT_IOS;
    
    bool result = authorize(&request);
    
    TEST_ASSERT_FALSE(result);
    TEST_ASSERT_NOT_NULL(get_authorize_error());
    TEST_ASSERT_EQUAL_STRING("Client version too low, upgrade please.", get_authorize_error());
}

void test_authorize_with_invalid_client_type(void)
{
    // 测试无效的客户端类型
    YAAuthorizeEventRequest request;
    request.version = 1;
    request.type = 99; // 无效类型
    
    bool result = authorize(&request);
    
    TEST_ASSERT_FALSE(result);
    TEST_ASSERT_NOT_NULL(get_authorize_error());
    TEST_ASSERT_EQUAL_STRING("Invalid client type", get_authorize_error());
}

void test_authorize_with_valid_ios_client(void)
{
    // 测试有效的iOS客户端
    YAAuthorizeEventRequest request;
    request.version = 1;
    request.type = CLIENT_IOS;
    
    bool result = authorize(&request);
    
    TEST_ASSERT_TRUE(result);
    TEST_ASSERT_NULL(get_authorize_error());
}

void test_authorize_with_valid_android_client(void)
{
    // 测试有效的Android客户端
    YAAuthorizeEventRequest request;
    request.version = 2;
    request.type = CLIENT_ANDROID;
    
    bool result = authorize(&request);
    
    TEST_ASSERT_TRUE(result);
    TEST_ASSERT_NULL(get_authorize_error());
}

void test_authorize_with_higher_version(void)
{
    // 测试更高版本
    YAAuthorizeEventRequest request;
    request.version = 100;
    request.type = CLIENT_IOS;
    
    bool result = authorize(&request);
    
    TEST_ASSERT_TRUE(result);
    TEST_ASSERT_NULL(get_authorize_error());
}

void test_get_authorize_error_after_success(void)
{
    // 测试成功授权后错误信息应为NULL
    YAAuthorizeEventRequest request;
    request.version = 1;
    request.type = CLIENT_IOS;
    
    authorize(&request);
    
    TEST_ASSERT_NULL(get_authorize_error());
}

void test_authorize_error_memory_management(void)
{
    // 测试错误信息的内存管理
    YAAuthorizeEventRequest request;
    
    // 先产生一个错误
    request.version = 0;
    request.type = CLIENT_IOS;
    authorize(&request);
    TEST_ASSERT_NOT_NULL(get_authorize_error());
    
    // 再产生另一个错误，之前的错误信息应该被释放
    request.version = 1;
    request.type = 99; // 无效类型
    authorize(&request);
    TEST_ASSERT_NOT_NULL(get_authorize_error());
    TEST_ASSERT_EQUAL_STRING("Invalid client type", get_authorize_error());
    
    // 成功授权，错误信息应该被清空
    request.version = 1;
    request.type = CLIENT_IOS;
    authorize(&request);
    TEST_ASSERT_NULL(get_authorize_error());
}

int main(void)
{
    UNITY_BEGIN();
    
    RUN_TEST(test_authorize_with_null_param);
    RUN_TEST(test_authorize_with_low_version);
    RUN_TEST(test_authorize_with_invalid_client_type);
    RUN_TEST(test_authorize_with_valid_ios_client);
    RUN_TEST(test_authorize_with_valid_android_client);
    RUN_TEST(test_authorize_with_higher_version);
    RUN_TEST(test_get_authorize_error_after_success);
    RUN_TEST(test_authorize_error_memory_management);
    
    return UNITY_END();
} 