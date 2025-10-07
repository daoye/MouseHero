#include <ctype.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "ya_config.h"

void ya_config_init(YA_Config *config)
{
    config->entries = NULL;
    config->count = 0;
    config->capacity = 0;
}

void ya_config_free(YA_Config *config)
{
    if (config->entries)
    {
        free(config->entries);
    }
    config->entries = NULL;
    config->count = config->capacity = 0;
}

static void trim_whitespace(char *str)
{
    if (!str) return;

    char *start = str;
    char *end;

    // 跳过开头的空白字符
    while (isspace((unsigned char)*start))
        start++;

    if (*start == '\0') {
        *str = '\0';
        return;
    }

    // 移动字符串到开头
    if (start != str) {
        memmove(str, start, strlen(start) + 1);
    }

    // 去除结尾的空白字符
    end = str + strlen(str) - 1;
    while (end > str && isspace((unsigned char)*end))
        end--;

    *(end + 1) = '\0';
}

static void remove_inline_comment(char *str)
{
    if (!str) return;

    char *comment_start;
    
    // 检查分号注释
    comment_start = strchr(str, ';');
    if (comment_start) {
        *comment_start = '\0';
    }
    
    // 检查井号注释
    comment_start = strchr(str, '#');
    if (comment_start) {
        *comment_start = '\0';
    }

    // 再次去除尾部空白
    trim_whitespace(str);
}

static void safe_strncpy(char *dest, const char *src, size_t size)
{
    if (!dest || !src || size == 0) return;
    
    strncpy(dest, src, size - 1);
    dest[size - 1] = '\0';
}

int ya_config_parse(const char *filename, YA_Config *config)
{
    FILE *file;
    char line[MAX_LINE_LENGTH];
    char current_section[MAX_SECTION_LENGTH] = "";

#ifdef _WIN32
    fopen_s(&file, filename, "r");
#else
    file = fopen(filename, "r");
#endif

    if (!file)
        return -1;

    while (fgets(line, sizeof(line), file))
    {
        char *start = line;

        // 去除首尾空白
        trim_whitespace(start);
        if (*start == ';' || *start == '#' || *start == '\0')
            continue;

        // 处理分段
        if (*start == '[')
        {
            char *end = strchr(start, ']');
            if (end)
            {
                *end = '\0';
                safe_strncpy(current_section, start + 1, sizeof(current_section));
                trim_whitespace(current_section);
            }
            continue;
        }

        // 分割键值对
        char *end = strchr(start, '=');
        if (end)
        {
            *end = '\0';
            char *key = start;
            char *value = end + 1;

            // 去除键的空白和注释
            trim_whitespace(key);

            // 去除值的空白和注释
            trim_whitespace(value);
            remove_inline_comment(value);

            // 移除值周围的引号
            if (*value == '"')
            {
                char *end_quote = strrchr(value + 1, '"');
                if (end_quote)
                {
                    value++;
                    *end_quote = '\0';
                }
            }

            // 扩展内存
            if (config->count >= config->capacity)
            {
                size_t new_capacity = config->capacity * 2 + 8;
                YA_Config_Entry *new_entries = realloc(config->entries, new_capacity * sizeof(YA_Config_Entry));
                if (!new_entries)
                {
                    fclose(file);
                    return -2;
                }
                config->entries = new_entries;
                config->capacity = new_capacity;
            }

            // 保存条目
            YA_Config_Entry *entry = &config->entries[config->count++];
            safe_strncpy(entry->section, current_section, sizeof(entry->section));
            safe_strncpy(entry->key, key, sizeof(entry->key));
            safe_strncpy(entry->value, value, sizeof(entry->value));
        }
    }

    fclose(file);
    return 0;
}

const char *ya_config_get(const YA_Config *config, const char *section, const char *key)
{
    if (!section || !key) return NULL;

    for (size_t i = 0; i < config->count; i++)
    {
        const YA_Config_Entry *entry = &config->entries[i];
        if (strcmp(entry->section, section) == 0 && strcmp(entry->key, key) == 0)
        {
            return entry->value;
        }
    }
    return NULL;
}

int ya_config_set(YA_Config *config, const char *section, const char *key, const char *value)
{
    if (!section || !key || !value) return -1;

    // 首先，尝试查找并更新现有条目
    for (size_t i = 0; i < config->count; i++)
    {
        YA_Config_Entry *entry = &config->entries[i];
        if (strcmp(entry->section, section) == 0 && strcmp(entry->key, key) == 0)
        {
            safe_strncpy(entry->value, value, sizeof(entry->value));
            return 0; // 更新成功
        }
    }

    // 如果没有找到，则添加新条目
    if (config->count >= config->capacity)
    {
        size_t new_capacity = config->capacity * 2 + 8;
        YA_Config_Entry *new_entries = realloc(config->entries, new_capacity * sizeof(YA_Config_Entry));
        if (!new_entries)
        {
            return -2; // 内存分配失败
        }
        config->entries = new_entries;
        config->capacity = new_capacity;
    }

    YA_Config_Entry *entry = &config->entries[config->count++];
    safe_strncpy(entry->section, section, sizeof(entry->section));
    safe_strncpy(entry->key, key, sizeof(entry->key));
    safe_strncpy(entry->value, value, sizeof(entry->value));

    return 0; // 添加成功
}

int ya_config_save(const YA_Config *config, const char *filename)
{
    FILE *file;
#ifdef _WIN32
    fopen_s(&file, filename, "w");
#else
    file = fopen(filename, "w");
#endif

    if (!file) return -1; // 打开文件失败

    char last_section[MAX_SECTION_LENGTH] = "";

    for (size_t i = 0; i < config->count; ++i)
    {
        const YA_Config_Entry *entry = &config->entries[i];
        if (strcmp(last_section, entry->section) != 0)
        {
            if (i > 0) {
                fprintf(file, "\n"); // 在新段前添加空行
            }
            fprintf(file, "[%s]\n", entry->section);
            safe_strncpy(last_section, entry->section, sizeof(last_section));
        }
        fprintf(file, "%s=%s\n", entry->key, entry->value);
    }

    fclose(file);
    return 0;
}
