#include "ya_server_http.h"
#include "ya_logger.h"
#include "ya_utils.h"
#include "mpack/mpack.h"
#include <event2/http.h>
#include <event2/buffer.h>
#include <string.h>
#include <stdlib.h>
#include <unistd.h>
#include "ya_server.h"
#include "ya_config.h"

static struct evhttp* g_http_server = NULL;

// 定义请求处理函数的类型
typedef void (*http_request_handler)(const char* request_data, size_t request_len, mpack_writer_t* writer);

// 前向声明
static void process_request(struct evhttp_request* req, http_request_handler handler);
static void handle_command(const char* request_data, size_t request_len, mpack_writer_t* writer);
static void handle_status(const char* request_data, size_t request_len, mpack_writer_t* writer);
static void handle_get_config(const char* request_data, size_t request_len, mpack_writer_t* writer);
static void handle_post_config(const char* request_data, size_t request_len, mpack_writer_t* writer);

// 发送msgpack格式的响应
static void send_msgpack_response(struct evhttp_request* req, int code, const char* reason, const char* data, size_t size) {
    struct evbuffer* buf = evbuffer_new();
    if (!buf) {
        evhttp_send_error(req, HTTP_INTERNAL, "Failed to create response buffer");
        return;
    }
    
    // 添加数据到响应缓冲区
    evbuffer_add(buf, data, size);
    
    // 设置响应头
    struct evkeyvalq* headers = evhttp_request_get_output_headers(req);
    evhttp_add_header(headers, "Content-Type", "application/x-msgpack");
    evhttp_send_reply(req, code, reason, buf);
    
    evbuffer_free(buf);
}

// 处理命令请求
static void handle_command_request(struct evhttp_request *req, void *arg) {
    if (evhttp_request_get_command(req) != EVHTTP_REQ_POST) {
        evhttp_send_error(req, HTTP_BADMETHOD, "Only POST method is allowed");
        return;
    }
    process_request(req, handle_command);
}

static void handle_command(const char* request_data, size_t request_len, mpack_writer_t* writer) {
    // TODO: 解析请求数据，执行命令
    mpack_start_map(writer, 1);
    mpack_write_cstr(writer, "result");
    mpack_write_cstr(writer, "success");
    mpack_finish_map(writer);
}

// 处理状态查询
static void handle_status_request(struct evhttp_request *req, void *arg) {
    if (evhttp_request_get_command(req) != EVHTTP_REQ_GET) {
        evhttp_send_error(req, HTTP_BADMETHOD, "Only GET method is allowed");
        return;
    }
    process_request(req, handle_status);
}

static void handle_status(const char* request_data, size_t request_len, mpack_writer_t* writer) {
    extern YA_ServerContext svr_context;
    extern YA_Config config;

    mpack_start_map(writer, 4);

    // 1. 服务器状态
    mpack_write_cstr(writer, "server_status");
    mpack_write_cstr(writer, ya_server_state_to_string(ya_server_get_state()));

    // 2. 监听地址
    mpack_write_cstr(writer, "listeners");
    mpack_start_map(writer, 3);
    mpack_write_cstr(writer, "session");
    mpack_write_cstr(writer, svr_context.session_addr ? svr_context.session_addr : "");
    mpack_write_cstr(writer, "command");
    mpack_write_cstr(writer, svr_context.command_addr ? svr_context.command_addr : "");
    mpack_write_cstr(writer, "http");
    mpack_write_cstr(writer, svr_context.http_addr ? svr_context.http_addr : "");
    mpack_finish_map(writer);

    // 3. 客户端列表
    mpack_write_cstr(writer, "connected_clients");
    mpack_start_array(writer, ya_client_get_count(&svr_context.client_manager));
    for (int i = 0; i < YA_CLIENT_HASH_SIZE; ++i) {
        for (ya_client_t* client = svr_context.client_manager.buckets[i]; client != NULL; client = client->next) {
            if (client->state == YA_CLIENT_ACTIVE) {
                char client_addr[INET_ADDRSTRLEN];
                ya_get_peer_addr(client->fd, client_addr, sizeof(client_addr));
                mpack_start_map(writer, 3);
                mpack_write_cstr(writer, "uid");
                mpack_write_u32(writer, client->uid);
                mpack_write_cstr(writer, "address");
                mpack_write_cstr(writer, client_addr);
                mpack_write_cstr(writer, "connected_at");
                mpack_write_u64(writer, (uint64_t)client->connected_at);
                mpack_finish_map(writer);
            }
        }
    }
    mpack_finish_array(writer);

    // 4. 日志路径
    mpack_write_cstr(writer, "log_path");
    const char* log_file = ya_config_get(&config, "logger", "file");
    mpack_write_cstr(writer, log_file ? log_file : "");

    mpack_finish_map(writer);
}

// 处理配置获取请求
static void handle_post_config(const char* request_data, size_t request_len, mpack_writer_t* writer) {
    extern YA_Config config;
    extern YA_ServerContext svr_context;

    mpack_reader_t reader;
    mpack_reader_init_data(&reader, request_data, request_len);

    // 期望是一个包含多个配置项的数组
    mpack_tag_t array_tag = mpack_read_tag(&reader);
    if (mpack_tag_type(&array_tag) != mpack_type_array) {
        mpack_start_map(writer, 2);
        mpack_write_cstr(writer, "result");
        mpack_write_cstr(writer, "error");
        mpack_write_cstr(writer, "message");
        mpack_write_cstr(writer, "Invalid request format: expected an array.");
        mpack_finish_map(writer);
        return;
    }

    uint32_t count = mpack_tag_array_count(&array_tag);
    for (uint32_t i = 0; i < count; ++i) {
        char section[MAX_SECTION_LENGTH] = {0};
        char key[MAX_KEY_LENGTH] = {0};
        char value[MAX_VALUE_LENGTH] = {0};

        mpack_expect_map(&reader);

        // 解析键值对，我们假设顺序是固定的: section, key, value
        char temp_key[MAX_KEY_LENGTH];

        // 读取 section
        mpack_expect_utf8_cstr(&reader, temp_key, sizeof(temp_key)); // 读取键 "section"
        mpack_expect_utf8_cstr(&reader, section, sizeof(section));   // 读取 section 的值

        // 读取 key
        mpack_expect_utf8_cstr(&reader, temp_key, sizeof(temp_key)); // 读取键 "key"
        mpack_expect_utf8_cstr(&reader, key, sizeof(key));           // 读取 key 的值

        // 读取 value
        mpack_expect_utf8_cstr(&reader, temp_key, sizeof(temp_key)); // 读取键 "value"
        mpack_expect_utf8_cstr(&reader, value, sizeof(value));       // 读取 value 的值

        mpack_done_map(&reader);

        if (ya_config_set(&config, section, key, value) != 0) {
            YA_LOG_WARN("Failed to set config: %s.%s = %s", section, key, value);
        }
    }

    if (mpack_reader_destroy(&reader) != mpack_ok) {
        mpack_start_map(writer, 2);
        mpack_write_cstr(writer, "result");
        mpack_write_cstr(writer, "error");
        mpack_write_cstr(writer, "message");
        mpack_write_cstr(writer, "Failed to parse request data.");
        mpack_finish_map(writer);
        return;
    }

    if (ya_config_save(&config, svr_context.config_file_path) != 0) {
        mpack_start_map(writer, 2);
        mpack_write_cstr(writer, "result");
        mpack_write_cstr(writer, "error");
        mpack_write_cstr(writer, "message");
        mpack_write_cstr(writer, "Failed to save configuration file.");
        mpack_finish_map(writer);
        return;
    }

    mpack_start_map(writer, 1);
    mpack_write_cstr(writer, "result");
    mpack_write_cstr(writer, "success");
    mpack_finish_map(writer);
}

static void handle_config_request(struct evhttp_request *req, void *arg) {
    enum evhttp_cmd_type method = evhttp_request_get_command(req);

    if (method == EVHTTP_REQ_GET) {
        process_request(req, handle_get_config);
    } else if (method == EVHTTP_REQ_POST) {
        process_request(req, handle_post_config);
    } else {
        evhttp_send_error(req, HTTP_BADMETHOD, "Method not allowed");
    }
}

static void handle_get_config(const char* request_data, size_t request_len, mpack_writer_t* writer) {
    extern YA_Config config;

    mpack_start_array(writer, config.count);
    for (size_t i = 0; i < config.count; ++i) {
        mpack_start_map(writer, 3);
        mpack_write_cstr(writer, "section");
        mpack_write_cstr(writer, config.entries[i].section);
        mpack_write_cstr(writer, "key");
        mpack_write_cstr(writer, config.entries[i].key);
        mpack_write_cstr(writer, "value");
        mpack_write_cstr(writer, config.entries[i].value);
        mpack_finish_map(writer);
    }
    mpack_finish_array(writer);
}

// 通用的HTTP GET请求处理函数


static void process_request(struct evhttp_request* req, http_request_handler handler) {
    const char* request_data = NULL;
    size_t request_len = 0;
    char* request_data_buffer = NULL;

    if (evhttp_request_get_command(req) == EVHTTP_REQ_POST) {
        struct evbuffer* buf = evhttp_request_get_input_buffer(req);
        request_len = evbuffer_get_length(buf);
        if (request_len > 0) {
            request_data_buffer = malloc(request_len);
            if (!request_data_buffer) {
                evhttp_send_error(req, HTTP_INTERNAL, "Failed to allocate memory for request body");
                return;
            }
            evbuffer_copyout(buf, request_data_buffer, request_len);
            request_data = request_data_buffer;
        }
    }

    mpack_writer_t writer;
    char* response_buffer = NULL;
    size_t response_size = 0;
    mpack_writer_init_growable(&writer, &response_buffer, &response_size);

    handler(request_data, request_len, &writer);

    if (request_data_buffer) {
        free(request_data_buffer);
    }

    if (mpack_writer_destroy(&writer) != mpack_ok) {
        evhttp_send_error(req, HTTP_INTERNAL, "Failed to encode response");
        return;
    }

    send_msgpack_response(req, HTTP_OK, "OK", response_buffer, response_size);
    MPACK_FREE(response_buffer);
}


extern YA_ServerContext svr_context;
extern YA_Config config;

int run_http_server(void) {
    if (!svr_context.http_addr || strlen(svr_context.http_addr) == 0) {
        YA_LOG_WARN("HTTP server listener not configured, server disabled.");
        return 0;
    }

    g_http_server = evhttp_new(svr_context.base);
    if (!g_http_server) {
        YA_LOG_ERROR("Failed to create HTTP server");
        return -1;
    }

    char* host = strdup(svr_context.http_addr);
    if (!host) {
        YA_LOG_ERROR("Failed to allocate memory for host");
        return -1;
    }

    char* port_str = strrchr(host, ':');
    if (port_str) {
        *port_str = '\0';
    }

    if (evhttp_bind_socket(g_http_server, 
                           host, 
                           ntohs(((struct sockaddr_in*)&svr_context.http_sock_addr)->sin_port)) != 0) {
        free(host);
        YA_LOG_ERROR("Failed to bind HTTP server to %s", svr_context.http_addr);
        evhttp_free(g_http_server);
        g_http_server = NULL;
        return -1;
    }

    free(host);

    // 设置允许的HTTP方法
    evhttp_set_allowed_methods(g_http_server, EVHTTP_REQ_GET | EVHTTP_REQ_POST);

    // 注册各个路径的处理函数
    evhttp_set_cb(g_http_server, "/command", handle_command_request, NULL);
    evhttp_set_cb(g_http_server, "/status", handle_status_request, NULL);
    evhttp_set_cb(g_http_server, "/config", handle_config_request, NULL);

    YA_LOG_INFO("HTTP server listening on %s", svr_context.http_addr);
    return 0;
}

void stop_http_server(void) {
    if (g_http_server) {
        evhttp_free(g_http_server);
        g_http_server = NULL;
        YA_LOG_INFO("HTTP server stopped.");
    }
} 