#pragma once

#include <event2/util.h>
#include <stdbool.h>
#include <stdint.h>
#include <time.h>

// 哈希表大小（使用质数以减少碰撞）
#define YA_CLIENT_HASH_SIZE 251

// 前置声明：每客户端鼠标滤波上下文
typedef struct ya_mouse_filter ya_mouse_filter_t;

// 客户端状态
typedef enum {
    YA_CLIENT_ACTIVE = 0,
    YA_CLIENT_DISCONNECTING = 1,
    YA_CLIENT_DISCONNECTED = 2
} ya_client_state_t;

// 客户端结构体
typedef struct ya_client {
    evutil_socket_t fd;          // 文件描述符
    uint32_t uid;               // 客户端唯一ID
    int command_index;          // 命令索引
    uint32_t ref_count;         // 引用计数
    ya_client_state_t state;    // 客户端状态
    struct bufferevent *bev;    // 事件缓冲区
    struct ya_client *next;     // 哈希冲突链表
    // 服务器端挂载的鼠标移动平滑上下文（按客户端隔离）
    ya_mouse_filter_t *mouse_filter;
    time_t connected_at;        // 连接建立时间（Unix 时间戳，秒）
} ya_client_t;

// 客户端管理器结构体
typedef struct {
    ya_client_t *buckets[YA_CLIENT_HASH_SIZE];  // 哈希桶
    uint32_t client_count;                      // 当前客户端数量
    uint32_t next_uid;                          // 下一个可用的UID
} ya_client_manager_t;

// 初始化客户端管理器
void ya_client_manager_init(ya_client_manager_t *manager);

// 清理客户端管理器
void ya_client_manager_cleanup(ya_client_manager_t *manager);

// 创建新客户端
ya_client_t *ya_client_create(ya_client_manager_t *manager, evutil_socket_t fd, struct bufferevent *bev);

// 查找客户端（通过UID）
ya_client_t *ya_client_find_by_uid(ya_client_manager_t *manager, uint32_t uid);

// 查找客户端（通过文件描述符）
ya_client_t *ya_client_find_by_fd(ya_client_manager_t *manager, evutil_socket_t fd);

// 增加引用计数
void ya_client_ref(ya_client_t *client);

// 减少引用计数
void ya_client_unref(ya_client_manager_t *manager, ya_client_t *client);

// 移除客户端
void ya_client_remove(ya_client_manager_t *manager, uint32_t uid);

// 获取客户端数量
uint32_t ya_client_get_count(ya_client_manager_t *manager); 