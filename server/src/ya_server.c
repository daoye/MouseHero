// #include <netdb.h>
#include <signal.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <fcntl.h>

#ifndef _WIN32
#include <netinet/in.h>
#ifdef _XOPEN_SOURCE_EXTENDED
#include <arpa/inet.h>
#endif
#include <sys/socket.h>
#endif

#include <event2/buffer.h>
#include <event2/bufferevent.h>
#include <event2/event.h>
#include <event2/listener.h>
#include <event2/util.h>
#include <event2/thread.h>
#include <event2/dns.h>

#include "ya_logger.h"
#include "ya_server.h"
#include "ya_server_command.h"
#include "ya_server_discover.h"
#include "ya_server_session.h"
#include "ya_server_http.h"
#include "ya_utils.h"

#ifdef USE_UINPUT
#include "input/backend/linux_uinput.h"
#include "input/backend/xkb_mapper.h"
#endif
#include "input/facade.h"
#include "input/keyboard/clipboard.h"

extern YA_ServerContext svr_context;
extern YA_Config config;

void fatal_cb(int code)
{
    printf("Event fatal error\n");
}

void stop()
{
#ifdef USE_UINPUT
    // 关闭输入后端
    xkbmap_free();
    input_linux_shutdown();
#endif

    if (svr_context.listener)
    {
        evconnlistener_free(svr_context.listener);
        svr_context.listener = NULL;
    }

    if (svr_context.command_fd)
    {
        close(svr_context.command_fd);
        svr_context.command_fd = 0;
    }

    // 停止HTTP服务
    stop_http_server();

    // 清理客户端管理器
    ya_client_manager_cleanup(&svr_context.client_manager);

    if (svr_context.base)
    {
        event_base_free(svr_context.base);
        svr_context.base = NULL;
    }

    if (svr_context.macs_csv)
    {
        free(svr_context.macs_csv);
        svr_context.macs_csv = NULL;
    }

    YA_LOG_DEBUG("Stopped");
}

static void signal_cb(evutil_socket_t sig, short events, void *user_data)
{
    YA_LOG_DEBUG("Stopping...");

    event_base_loopexit(svr_context.base, 0);
}



// 服务器状态管理实现
void ya_server_set_state(ya_server_state_t new_state) {
    ya_server_state_t old_state = svr_context.state;
    svr_context.state = new_state;
    
    YA_LOG_INFO("Server state changed: %s -> %s", 
        ya_server_state_to_string(old_state),
        ya_server_state_to_string(new_state));
}

ya_server_state_t ya_server_get_state(void) {
    return svr_context.state;
}

const char* ya_server_state_to_string(ya_server_state_t state) {
    switch (state) {
        case YA_SERVER_INIT: return "INIT";
        case YA_SERVER_RUNNING: return "RUNNING";
        case YA_SERVER_STOPPING: return "STOPPING";
        case YA_SERVER_STOPPED: return "STOPPED";
        default: return "UNKNOWN";
    }
}

// 组件状态管理实现
void ya_server_set_component_ready(ya_server_components_state_t *state, const char *component, bool ready) {
    if (strcmp(component, "session") == 0) {
        state->session_server_ready = ready;
    } else if (strcmp(component, "command") == 0) {
        state->command_server_ready = ready;
    } else if (strcmp(component, "http") == 0) {
        state->http_server_ready = ready;
    } else if (strcmp(component, "discovery") == 0) {
        state->discovery_ready = ready;
    }

    // 检查是否所有组件都已就绪
    if (ready && ya_server_all_components_ready(state)) {
        ya_server_set_state(YA_SERVER_RUNNING);
    }
}

bool ya_server_is_component_ready(const ya_server_components_state_t *state, const char *component) {
    if (strcmp(component, "session") == 0) {
        return state->session_server_ready;
    } else if (strcmp(component, "command") == 0) {
        return state->command_server_ready;
    } else if (strcmp(component, "http") == 0) {
        return state->http_server_ready;
    } else if (strcmp(component, "discovery") == 0) {
        return state->discovery_ready;
    }
    return false;
}

bool ya_server_all_components_ready(const ya_server_components_state_t *state) {
    return state->session_server_ready && 
           state->command_server_ready && 
           state->http_server_ready && 
           state->discovery_ready;
}

// 统计信息管理实现
void ya_server_stats_inc_connections(void) {
    svr_context.stats.total_connections++;
    svr_context.stats.active_connections++;
}

void ya_server_stats_dec_connections(void) {
    if (svr_context.stats.active_connections > 0) {
        svr_context.stats.active_connections--;
    }
}

void ya_server_stats_inc_commands(bool success) {
    svr_context.stats.total_commands++;
    if (!success) {
        svr_context.stats.failed_commands++;
    }
}

size_t ya_server_stats_pack(const ya_server_stats_t *stats, char *buffer, size_t buffer_size) {
    mpack_writer_t writer;
    mpack_writer_init(&writer, buffer, buffer_size);

    mpack_start_map(&writer, 4);
    
    mpack_write_cstr(&writer, "total_connections");
    mpack_write_u32(&writer, stats->total_connections);
    
    mpack_write_cstr(&writer, "active_connections");
    mpack_write_u32(&writer, stats->active_connections);
    
    mpack_write_cstr(&writer, "total_commands");
    mpack_write_u32(&writer, stats->total_commands);
    
    mpack_write_cstr(&writer, "failed_commands");
    mpack_write_u32(&writer, stats->failed_commands);
    
    mpack_finish_map(&writer);

    size_t size = mpack_writer_buffer_used(&writer);
    mpack_writer_destroy(&writer);

    return size;
}

// 修改start_server函数，添加状态管理
int start_server() {
#ifdef _WIN32
    WSADATA wsa_data;
    if (WSAStartup(MAKEWORD(2, 2), &wsa_data) != 0) {
        return -1;
    }
#endif

    // 初始化服务器状态
    memset(&svr_context.stats, 0, sizeof(svr_context.stats));
    memset(&svr_context.components_state, 0, sizeof(svr_context.components_state));
    svr_context.macs_csv = NULL;
    ya_server_set_state(YA_SERVER_INIT);

#ifdef USE_UINPUT
    // 初始化输入后端（Linux uinput）
    if (input_linux_init() != INPUT_BACKEND_OK) {
        YA_LOG_WARN("Failed to initialize input backend, input control may be unavailable");
    } else {
        YA_LOG_INFO("Input backend initialized successfully");
    }
    
    // Initialize xkb mapping for character→key translation
    if (xkbmap_init_auto() == 0) {
        YA_LOG_INFO("XKB mapping initialized: layout source = %s", xkbmap_get_layout_source());
    } else {
        YA_LOG_WARN("XKB mapping initialization failed; character→key mapping unavailable");
    }
#endif

    // Initialize clipboard helper (reads config)
    clipboard_helper_init();
    
    event_enable_debug_mode();
    event_set_fatal_callback(fatal_cb);

    svr_context.base = event_base_new();
    if (!svr_context.base) {
        return -1;
    }

    // 初始化客户端管理器
    ya_client_manager_init(&svr_context.client_manager);

    struct event *signal_event = evsignal_new(svr_context.base, SIGINT, signal_cb, NULL);
    event_add(signal_event, NULL);

    // 启动各个组件
    // 获取并缓存一次MAC地址列表
    svr_context.macs_csv = ya_get_macs_csv();
    if (!svr_context.macs_csv)
    {
        YA_LOG_WARN("No MAC addresses found or failed to get MACs");
    }

    YA_LOG_TRACE("MACs %s", svr_context.macs_csv);

    if (run_session_server() < 0) {
        stop();
        return -1;
    }
    ya_server_set_component_ready(&svr_context.components_state, "session", true);

    // 设置HTTP服务器地址
    svr_context.http_addr = ya_config_get(&config, "http", "listener");
    if (svr_context.http_addr) {
        memset(&svr_context.http_sock_addr, 0, sizeof(svr_context.http_sock_addr));
        if (evutil_parse_sockaddr_port(svr_context.http_addr, (struct sockaddr*)&svr_context.http_sock_addr, 
            &(int){sizeof(svr_context.http_sock_addr)}) < 0) {
            YA_LOG_ERROR("Failed to parse HTTP listen address: %s", svr_context.http_addr);
        }
    }

    if (run_command_server() < 0) {
        stop();
        return -1;
    }
    ya_server_set_component_ready(&svr_context.components_state, "command", true);

    // 启动发现服务
    if (run_discovery_server() == 0) {
        ya_server_set_component_ready(&svr_context.components_state, "discovery", true);
    }

    // 启动HTTP服务
    if (run_http_server() < 0) {
        YA_LOG_ERROR("Failed to start HTTP server");
        stop();
        return -1;
    }
    ya_server_set_component_ready(&svr_context.components_state, "http", true);

    event_base_dispatch(svr_context.base);

    ya_server_set_state(YA_SERVER_STOPPING);
    stop();
    ya_server_set_state(YA_SERVER_STOPPED);

    return 0;
}

// 为了向后兼容的旧接口实现
struct ya_client *get_client_by_id(uint32_t uid)
{
    return (struct ya_client *)ya_client_find_by_uid(&svr_context.client_manager, uid);
}

struct ya_client *get_client_by_fd(evutil_socket_t fd)
{
    return (struct ya_client *)ya_client_find_by_fd(&svr_context.client_manager, fd);
}

struct ya_client *new_client_and_append_to_context(evutil_socket_t fd)
{
    return (struct ya_client *)ya_client_create(&svr_context.client_manager, fd, NULL);
}

void remove_client(uint32_t uid)
{
    ya_client_remove(&svr_context.client_manager, uid);
}
