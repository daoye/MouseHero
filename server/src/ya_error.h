#ifndef YA_ERROR_H
#define YA_ERROR_H

#include <stddef.h>

// 错误码定义
typedef enum {
    YA_OK = 0,                    // 成功
    YA_ERR_UNKNOWN = -1,         // 未知错误
    
    // 系统错误 (-100 ~ -199)
    YA_ERR_MEMORY = -100,        // 内存分配错误
    YA_ERR_TIMEOUT = -101,       // 超时错误
    YA_ERR_BUSY = -102,          // 系统忙
    YA_ERR_INTERRUPTED = -103,   // 操作被中断
    
    // 配置错误 (-200 ~ -299)
    YA_ERR_CONFIG_INVALID = -200,    // 配置无效
    YA_ERR_CONFIG_NOT_FOUND = -201,  // 配置项不存在
    YA_ERR_CONFIG_TYPE = -202,       // 配置类型错误
    
    // 网络错误 (-300 ~ -399)
    YA_ERR_NETWORK = -300,           // 网络错误
    YA_ERR_CONNECTION = -301,        // 连接错误
    YA_ERR_SOCKET = -302,           // Socket错误
    YA_ERR_PROTOCOL = -303,         // 协议错误
    
    // 会话错误 (-400 ~ -499)
    YA_ERR_SESSION_INVALID = -400,   // 无效会话
    YA_ERR_SESSION_EXPIRED = -401,   // 会话过期
    YA_ERR_SESSION_LIMIT = -402,     // 会话数量超限
    
    // 命令错误 (-500 ~ -599)
    YA_ERR_CMD_INVALID = -500,       // 无效命令
    YA_ERR_CMD_PARAMS = -501,        // 命令参数错误
    YA_ERR_CMD_UNSUPPORTED = -502,   // 不支持的命令
    YA_ERR_CMD_FAILED = -503,        // 命令执行失败
    
    // 权限错误 (-600 ~ -699)
    YA_ERR_PERMISSION = -600,        // 权限错误
    YA_ERR_AUTH_FAILED = -601,       // 认证失败
    
    // 资源错误 (-700 ~ -799)
    YA_ERR_RESOURCE_BUSY = -700,     // 资源忙
    YA_ERR_RESOURCE_LIMIT = -701,    // 资源超限
    YA_ERR_RESOURCE_NOT_FOUND = -702 // 资源不存在
} ya_error_code_t;

// 错误堆栈大小
#define YA_ERROR_STACK_SIZE 64
#define YA_ERROR_MSG_SIZE 256

// 错误信息结构
typedef struct {
    ya_error_code_t code;            // 错误码
    char message[YA_ERROR_MSG_SIZE]; // 错误信息
    char file[256];                  // 错误发生的文件
    int line;                        // 错误发生的行号
    char function[64];               // 错误发生的函数
} ya_error_info_t;

// 错误上下文结构
typedef struct {
    ya_error_info_t stack[YA_ERROR_STACK_SIZE];  // 错误堆栈
    size_t stack_size;                           // 当前堆栈大小
    ya_error_code_t last_error;                  // 最后一个错误码
} ya_error_context_t;

// 初始化错误处理系统
void ya_error_init(void);

// 清理错误处理系统
void ya_error_cleanup(void);

// 设置错误
void ya_error_set(ya_error_code_t code, const char* message, 
                 const char* file, int line, const char* function);

// 获取最后一个错误码
ya_error_code_t ya_error_get_last(void);

// 获取错误信息
const char* ya_error_get_message(ya_error_code_t code);

// 获取错误堆栈
const ya_error_info_t* ya_error_get_stack(size_t* size);

// 清除错误堆栈
void ya_error_clear(void);

// 错误设置宏（方便使用）
#define YA_SET_ERROR(code, message) \
    ya_error_set(code, message, __FILE__, __LINE__, __func__)

// 错误检查宏
#define YA_CHECK_ERROR(code) \
    do { \
        if (code != YA_OK) { \
            return code; \
        } \
    } while (0)

#endif // YA_ERROR_H 