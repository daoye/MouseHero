#include "ya_error.h"
#include "ya_logger.h"
#include <string.h>
#include <stdio.h>
#include <pthread.h>

// 线程局部存储的错误上下文
static __thread ya_error_context_t* error_context = NULL;

// 错误码对应的错误信息
#define ERROR_MESSAGE_COUNT 30

// 错误码到数组索引的映射函数
static inline size_t error_code_to_index(ya_error_code_t code) {
    switch (code) {
        case YA_OK: return 0;
        case YA_ERR_UNKNOWN: return 1;
        case YA_ERR_MEMORY: return 2;
        case YA_ERR_TIMEOUT: return 3;
        case YA_ERR_BUSY: return 4;
        case YA_ERR_INTERRUPTED: return 5;
        case YA_ERR_CONFIG_INVALID: return 6;
        case YA_ERR_CONFIG_NOT_FOUND: return 7;
        case YA_ERR_CONFIG_TYPE: return 8;
        case YA_ERR_NETWORK: return 9;
        case YA_ERR_CONNECTION: return 10;
        case YA_ERR_SOCKET: return 11;
        case YA_ERR_PROTOCOL: return 12;
        case YA_ERR_SESSION_INVALID: return 13;
        case YA_ERR_SESSION_EXPIRED: return 14;
        case YA_ERR_SESSION_LIMIT: return 15;
        case YA_ERR_CMD_INVALID: return 16;
        case YA_ERR_CMD_PARAMS: return 17;
        case YA_ERR_CMD_UNSUPPORTED: return 18;
        case YA_ERR_CMD_FAILED: return 19;
        case YA_ERR_PERMISSION: return 20;
        case YA_ERR_AUTH_FAILED: return 21;
        case YA_ERR_RESOURCE_BUSY: return 22;
        case YA_ERR_RESOURCE_LIMIT: return 23;
        case YA_ERR_RESOURCE_NOT_FOUND: return 24;
        default: return ERROR_MESSAGE_COUNT;
    }
}

static const char* error_messages[ERROR_MESSAGE_COUNT] = {
    "Success",                    // YA_OK
    "Unknown error",             // YA_ERR_UNKNOWN
    
    // 系统错误
    "Memory allocation failed",   // YA_ERR_MEMORY
    "Operation timed out",        // YA_ERR_TIMEOUT
    "System is busy",            // YA_ERR_BUSY
    "Operation interrupted",      // YA_ERR_INTERRUPTED
    
    // 配置错误
    "Invalid configuration",      // YA_ERR_CONFIG_INVALID
    "Configuration item not found", // YA_ERR_CONFIG_NOT_FOUND
    "Configuration type mismatch", // YA_ERR_CONFIG_TYPE
    
    // 网络错误
    "Network error",             // YA_ERR_NETWORK
    "Connection error",          // YA_ERR_CONNECTION
    "Socket error",             // YA_ERR_SOCKET
    "Protocol error",           // YA_ERR_PROTOCOL
    
    // 会话错误
    "Invalid session",          // YA_ERR_SESSION_INVALID
    "Session expired",          // YA_ERR_SESSION_EXPIRED
    "Session limit exceeded",    // YA_ERR_SESSION_LIMIT
    
    // 命令错误
    "Invalid command",          // YA_ERR_CMD_INVALID
    "Invalid command parameters", // YA_ERR_CMD_PARAMS
    "Unsupported command",      // YA_ERR_CMD_UNSUPPORTED
    "Command execution failed",  // YA_ERR_CMD_FAILED
    
    // 权限错误
    "Permission denied",        // YA_ERR_PERMISSION
    "Authentication failed",    // YA_ERR_AUTH_FAILED
    
    // 资源错误
    "Resource is busy",         // YA_ERR_RESOURCE_BUSY
    "Resource limit exceeded",  // YA_ERR_RESOURCE_LIMIT
    "Resource not found"        // YA_ERR_RESOURCE_NOT_FOUND
};

void ya_error_init(void) {
    if (error_context == NULL) {
        error_context = (ya_error_context_t*)calloc(1, sizeof(ya_error_context_t));
        if (error_context == NULL) {
            YA_LOG_ERROR("Failed to allocate error context");
            return;
        }
    }
}

void ya_error_cleanup(void) {
    if (error_context != NULL) {
        free(error_context);
        error_context = NULL;
    }
}

void ya_error_set(ya_error_code_t code, const char* message,
                 const char* file, int line, const char* function) {
    if (error_context == NULL) {
        ya_error_init();
        if (error_context == NULL) {
            return;
        }
    }

    // 更新最后一个错误码
    error_context->last_error = code;

    // 如果堆栈已满，移除最早的错误
    if (error_context->stack_size >= YA_ERROR_STACK_SIZE) {
        memmove(&error_context->stack[0], &error_context->stack[1],
                (YA_ERROR_STACK_SIZE - 1) * sizeof(ya_error_info_t));
        error_context->stack_size = YA_ERROR_STACK_SIZE - 1;
    }

    // 添加新的错误信息到堆栈
    ya_error_info_t* info = &error_context->stack[error_context->stack_size];
    info->code = code;
    strncpy(info->message, message ? message : "", YA_ERROR_MSG_SIZE - 1);
    info->message[YA_ERROR_MSG_SIZE - 1] = '\0';
    strncpy(info->file, file ? file : "", sizeof(info->file) - 1);
    info->file[sizeof(info->file) - 1] = '\0';
    info->line = line;
    strncpy(info->function, function ? function : "", sizeof(info->function) - 1);
    info->function[sizeof(info->function) - 1] = '\0';

    error_context->stack_size++;

    // 记录错误日志
    YA_LOG_ERROR("Error occurred: [%d] %s at %s:%d (%s)",
                 code, message, file, line, function);
}

ya_error_code_t ya_error_get_last(void) {
    return error_context ? error_context->last_error : YA_ERR_UNKNOWN;
}

const char* ya_error_get_message(ya_error_code_t code) {
    size_t index = error_code_to_index(code);
    if (index >= ERROR_MESSAGE_COUNT) {
        return "Invalid error code";
    }
    return error_messages[index];
}

const ya_error_info_t* ya_error_get_stack(size_t* size) {
    if (error_context == NULL || size == NULL) {
        if (size) {
            *size = 0;
        }
        return NULL;
    }

    *size = error_context->stack_size;
    return error_context->stack;
}

void ya_error_clear(void) {
    if (error_context != NULL) {
        error_context->stack_size = 0;
        error_context->last_error = YA_OK;
    }
} 