#pragma once

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define MAX_LINE_LENGTH 256
#define MAX_SECTION_LENGTH 64
#define MAX_KEY_LENGTH 64
#define MAX_VALUE_LENGTH 256

typedef struct
{
    char section[MAX_SECTION_LENGTH];
    char key[MAX_KEY_LENGTH];
    char value[MAX_VALUE_LENGTH];
} YA_Config_Entry;

typedef struct
{
    YA_Config_Entry *entries;
    size_t count;
    size_t capacity;
} YA_Config;

void ya_config_init(YA_Config *config);

void ya_config_free(YA_Config *config);

int ya_config_parse(const char *filename, YA_Config *config);

const char *ya_config_get(const YA_Config *config, const char *section, const char *key);

int ya_config_set(YA_Config *config, const char *section, const char *key, const char *value);

int ya_config_save(const YA_Config *config, const char *filename);
