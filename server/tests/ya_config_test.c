#include "unity.h"
#include "ya_config.h"
#include <stdio.h>
#include <string.h>

static YA_Config config;
static const char *TEST_CONFIG_FILE = "test_config.conf";

void setUp(void) {
    ya_config_init(&config);
}

void tearDown(void) {
    ya_config_free(&config);
    remove(TEST_CONFIG_FILE);  // 清理测试配置文件
}

// 辅助函数：创建测试配置文件
static void create_test_config_file(const char *content) {
    FILE *file = fopen(TEST_CONFIG_FILE, "w");
    TEST_ASSERT_NOT_NULL_MESSAGE(file, "Failed to create test config file");
    fprintf(file, "%s", content);
    fclose(file);
}

void test_config_init(void) {
    TEST_ASSERT_NULL(config.entries);
    TEST_ASSERT_EQUAL_size_t(0, config.count);
    TEST_ASSERT_EQUAL_size_t(0, config.capacity);
}

void test_config_get_nonexistent(void) {
    const char *value = ya_config_get(&config, "section", "key");
    TEST_ASSERT_NULL(value);
}

void test_config_parse_empty_file(void) {
    create_test_config_file("");
    int result = ya_config_parse(TEST_CONFIG_FILE, &config);
    TEST_ASSERT_EQUAL_INT(0, result);
    TEST_ASSERT_EQUAL_size_t(0, config.count);
}

void test_config_parse_normal(void) {
    const char *test_config = 
        "[section1]\n"
        "key1=value1\n"
        "key2=value2\n"
        "\n"
        "[section2]\n"
        "key3=value3\n";
    
    create_test_config_file(test_config);
    int result = ya_config_parse(TEST_CONFIG_FILE, &config);
    
    TEST_ASSERT_EQUAL_INT(0, result);
    TEST_ASSERT_EQUAL_size_t(3, config.count);
    
    const char *value1 = ya_config_get(&config, "section1", "key1");
    const char *value2 = ya_config_get(&config, "section1", "key2");
    const char *value3 = ya_config_get(&config, "section2", "key3");
    
    TEST_ASSERT_NOT_NULL(value1);
    TEST_ASSERT_NOT_NULL(value2);
    TEST_ASSERT_NOT_NULL(value3);
    TEST_ASSERT_EQUAL_STRING("value1", value1);
    TEST_ASSERT_EQUAL_STRING("value2", value2);
    TEST_ASSERT_EQUAL_STRING("value3", value3);
}

void test_config_parse_with_comments(void) {
    const char *test_config = 
        "; This is a comment\n"
        "# This is another comment\n"
        "[section1]\n"
        "key1=value1 ; inline comment\n"
        "key2=value2 # another inline comment\n";
    
    create_test_config_file(test_config);
    int result = ya_config_parse(TEST_CONFIG_FILE, &config);
    
    TEST_ASSERT_EQUAL_INT(0, result);
    TEST_ASSERT_EQUAL_size_t(2, config.count);
    
    const char *value1 = ya_config_get(&config, "section1", "key1");
    const char *value2 = ya_config_get(&config, "section1", "key2");
    
    TEST_ASSERT_EQUAL_STRING("value1", value1);
    TEST_ASSERT_EQUAL_STRING("value2", value2);
}

void test_config_parse_with_whitespace(void) {
    const char *test_config = 
        "  [section1]  \n"
        "  key1 = value1  \n"
        "\tkey2\t=\tvalue2\t\n";
    
    create_test_config_file(test_config);
    int result = ya_config_parse(TEST_CONFIG_FILE, &config);
    
    TEST_ASSERT_EQUAL_INT(0, result);
    TEST_ASSERT_EQUAL_size_t(2, config.count);
    
    const char *value1 = ya_config_get(&config, "section1", "key1");
    const char *value2 = ya_config_get(&config, "section1", "key2");
    
    TEST_ASSERT_EQUAL_STRING("value1", value1);
    TEST_ASSERT_EQUAL_STRING("value2", value2);
}

void test_config_parse_with_quotes(void) {
    const char *test_config = 
        "[section1]\n"
        "key1=\"quoted value\"\n"
        "key2=\"value with spaces\"\n";
    
    create_test_config_file(test_config);
    int result = ya_config_parse(TEST_CONFIG_FILE, &config);
    
    TEST_ASSERT_EQUAL_INT(0, result);
    TEST_ASSERT_EQUAL_size_t(2, config.count);
    
    const char *value1 = ya_config_get(&config, "section1", "key1");
    const char *value2 = ya_config_get(&config, "section1", "key2");
    
    TEST_ASSERT_EQUAL_STRING("quoted value", value1);
    TEST_ASSERT_EQUAL_STRING("value with spaces", value2);
}

void test_config_parse_invalid_file(void) {
    int result = ya_config_parse("nonexistent.conf", &config);
    TEST_ASSERT_EQUAL_INT(-1, result);
}

void test_config_parse_memory_growth(void) {
    // 创建一个包含多个条目的配置文件，测试内存增长
    char test_config[4096] = "";
    strcat(test_config, "[section]\n");
    for(int i = 0; i < 20; i++) {
        char line[64];
        snprintf(line, sizeof(line), "key%d=value%d\n", i, i);
        strcat(test_config, line);
    }
    
    create_test_config_file(test_config);
    int result = ya_config_parse(TEST_CONFIG_FILE, &config);
    
    TEST_ASSERT_EQUAL_INT(0, result);
    TEST_ASSERT_EQUAL_size_t(20, config.count);
    TEST_ASSERT_TRUE(config.capacity >= config.count);
    
    // 验证所有值都正确保存
    for(int i = 0; i < 20; i++) {
        char key[64], expected_value[64];
        snprintf(key, sizeof(key), "key%d", i);
        snprintf(expected_value, sizeof(expected_value), "value%d", i);
        
        const char *value = ya_config_get(&config, "section", key);
        TEST_ASSERT_NOT_NULL(value);
        TEST_ASSERT_EQUAL_STRING(expected_value, value);
    }
}

void test_config_parse_max_lengths(void) {
    // 创建一个测试长度限制的配置文件
    char section[MAX_SECTION_LENGTH + 10];
    char key[MAX_KEY_LENGTH + 10];
    char value[MAX_VALUE_LENGTH + 10];
    char truncated_section[MAX_SECTION_LENGTH];
    char truncated_key[MAX_KEY_LENGTH];
    
    // 填充测试数据
    memset(section, 'S', sizeof(section) - 1);
    memset(key, 'K', sizeof(key) - 1);
    memset(value, 'V', sizeof(value) - 1);
    section[sizeof(section) - 1] = '\0';
    key[sizeof(key) - 1] = '\0';
    value[sizeof(value) - 1] = '\0';
    
    // 创建截断后的版本
    memset(truncated_section, 'S', MAX_SECTION_LENGTH - 1);
    memset(truncated_key, 'K', MAX_KEY_LENGTH - 1);
    truncated_section[MAX_SECTION_LENGTH - 1] = '\0';
    truncated_key[MAX_KEY_LENGTH - 1] = '\0';
    
    char test_config[MAX_LINE_LENGTH * 2];
    snprintf(test_config, sizeof(test_config), 
             "[%s]\n%s=%s\n", 
             section, key, value);
    
    create_test_config_file(test_config);
    int result = ya_config_parse(TEST_CONFIG_FILE, &config);
    
    TEST_ASSERT_EQUAL_INT(0, result);
    TEST_ASSERT_EQUAL_size_t(1, config.count);
    
    const char *stored_value = ya_config_get(&config, truncated_section, truncated_key);
    TEST_ASSERT_NOT_NULL(stored_value);
    
    // 验证存储的值被正确截断
    TEST_ASSERT_TRUE(strlen(stored_value) <= MAX_VALUE_LENGTH - 1);
    TEST_ASSERT_TRUE(strlen(stored_value) > 0);
    
    // 验证所有字符都是 'V'
    for (size_t i = 0; i < strlen(stored_value); i++) {
        TEST_ASSERT_EQUAL_CHAR('V', stored_value[i]);
    }
}

void test_config_set_new_value(void) {
    int result = ya_config_set(&config, "new_section", "new_key", "new_value");
    TEST_ASSERT_EQUAL_INT(0, result);
    TEST_ASSERT_EQUAL_size_t(1, config.count);
    const char *value = ya_config_get(&config, "new_section", "new_key");
    TEST_ASSERT_EQUAL_STRING("new_value", value);
}

void test_config_set_update_existing_value(void) {
    ya_config_set(&config, "section", "key", "initial_value");
    int result = ya_config_set(&config, "section", "key", "updated_value");
    TEST_ASSERT_EQUAL_INT(0, result);
    TEST_ASSERT_EQUAL_size_t(1, config.count); // Count should not change
    const char *value = ya_config_get(&config, "section", "key");
    TEST_ASSERT_EQUAL_STRING("updated_value", value);
}

void test_config_set_memory_growth(void) {
    for (int i = 0; i < 20; i++) {
        char key[16];
        char value[16];
        sprintf(key, "key%d", i);
        sprintf(value, "value%d", i);
        int result = ya_config_set(&config, "growth_section", key, value);
        TEST_ASSERT_EQUAL_INT(0, result);
    }
    TEST_ASSERT_EQUAL_size_t(20, config.count);
    TEST_ASSERT_TRUE(config.capacity >= 20);
}

void test_config_save_and_reload(void) {
    // 1. Set some config values
    ya_config_set(&config, "section1", "key1", "value1");
    ya_config_set(&config, "section2", "key2", "value2");

    // 2. Save to file
    int save_result = ya_config_save(&config, TEST_CONFIG_FILE);
    TEST_ASSERT_EQUAL_INT(0, save_result);

    // 3. Create a new config, parse the saved file, and verify
    YA_Config new_config;
    ya_config_init(&new_config);
    int parse_result = ya_config_parse(TEST_CONFIG_FILE, &new_config);
    TEST_ASSERT_EQUAL_INT(0, parse_result);
    TEST_ASSERT_EQUAL_size_t(2, new_config.count);

    const char *value1 = ya_config_get(&new_config, "section1", "key1");
    const char *value2 = ya_config_get(&new_config, "section2", "key2");
    TEST_ASSERT_EQUAL_STRING("value1", value1);
    TEST_ASSERT_EQUAL_STRING("value2", value2);

    ya_config_free(&new_config);
}

void test_config_save_empty_config(void) {
    int save_result = ya_config_save(&config, TEST_CONFIG_FILE);
    TEST_ASSERT_EQUAL_INT(0, save_result);

    // Verify the file is empty or contains no entries
    FILE *file = fopen(TEST_CONFIG_FILE, "r");
    TEST_ASSERT_NOT_NULL(file);
    fseek(file, 0, SEEK_END);
    long size = ftell(file);
    fclose(file);
    TEST_ASSERT_EQUAL_INT(0, size);
}

int main(void) {
    UNITY_BEGIN();
    
    // 基本功能测试
    RUN_TEST(test_config_init);
    RUN_TEST(test_config_get_nonexistent);
    RUN_TEST(test_config_parse_empty_file);
    RUN_TEST(test_config_parse_normal);
    
    // 特殊情况测试
    RUN_TEST(test_config_parse_with_comments);
    RUN_TEST(test_config_parse_with_whitespace);
    RUN_TEST(test_config_parse_with_quotes);
    RUN_TEST(test_config_parse_invalid_file);
    
    // 边界条件测试
    RUN_TEST(test_config_parse_memory_growth);
    RUN_TEST(test_config_parse_max_lengths);

    // ya_config_set and ya_config_save tests
    RUN_TEST(test_config_set_new_value);
    RUN_TEST(test_config_set_update_existing_value);
    RUN_TEST(test_config_set_memory_growth);
    RUN_TEST(test_config_save_and_reload);
    RUN_TEST(test_config_save_empty_config);
    
    return UNITY_END();
} 