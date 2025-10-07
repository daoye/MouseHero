#pragma once

#include <event2/util.h>
#include <stdbool.h>
#include <stdint.h>

#include "ya_config.h"
#include "ya_client_manager.h"

// 服务器状态枚举
typedef enum {
    YA_SERVER_INIT,       // 初始化状态
    YA_SERVER_RUNNING,    // 正在运行
    YA_SERVER_STOPPING,   // 正在停止
    YA_SERVER_STOPPED     // 已停止
} ya_server_state_t;

// 服务器组件状态
typedef struct {
    bool session_server_ready;  // 会话服务器就绪状态
    bool command_server_ready;  // 命令服务器就绪状态
    bool http_server_ready;     // HTTP服务器就绪状态
    bool discovery_ready;       // 发现服务就绪状态
} ya_server_components_state_t;

// 服务器统计信息
typedef struct {
    uint32_t total_connections;     // 总连接数
    uint32_t active_connections;    // 活动连接数
    uint32_t total_commands;        // 总命令数
    uint32_t failed_commands;       // 失败命令数
} ya_server_stats_t;

typedef struct
{
    const char* config_file_path;
    struct event_base *base;    // libevent事件基础

    // 会话服务器配置
    const char *session_addr;
    struct sockaddr_in session_sock_addr;
    struct evconnlistener *listener;

    // 命令服务器配置
    const char *command_addr;
    struct sockaddr_in command_sock_addr;
    int command_fd;
    int uid_token;

    // HTTP服务器配置
    const char *http_addr;
    struct sockaddr_storage http_sock_addr;

    // 客户端管理
    ya_client_manager_t client_manager;

    // 服务器状态
    ya_server_state_t state;
    ya_server_components_state_t components_state;

    // 统计信息
    ya_server_stats_t stats;

    // 逗号拼接的网卡MAC地址
    char *macs_csv;
} YA_ServerContext;

#ifndef GLOBAL_VARS
#define GLOBAL_VARS
extern YA_ServerContext svr_context;
extern YA_Config config;
#endif

// 服务器状态管理函数
void ya_server_set_state(ya_server_state_t new_state);
ya_server_state_t ya_server_get_state(void);
const char* ya_server_state_to_string(ya_server_state_t state);

// 组件状态管理
void ya_server_set_component_ready(ya_server_components_state_t *state, const char *component, bool ready);
bool ya_server_is_component_ready(const ya_server_components_state_t *state, const char *component);
bool ya_server_all_components_ready(const ya_server_components_state_t *state);

// 统计信息管理
void ya_server_stats_inc_connections(void);
void ya_server_stats_dec_connections(void);
void ya_server_stats_inc_commands(bool success);

// 将统计信息序列化为MessagePack格式
size_t ya_server_stats_pack(const ya_server_stats_t *stats, char *buffer, size_t buffer_size);

// 废弃的旧接口，为了向后兼容暂时保留，但返回新的类型
ya_client_t *get_client_by_id(uint32_t uid);
ya_client_t *get_client_by_fd(evutil_socket_t fd);
ya_client_t *new_client_and_append_to_context(evutil_socket_t fd);
void remove_client(uint32_t uid);



int start_server();
