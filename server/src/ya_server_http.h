#ifndef YA_HTTP_H
#define YA_HTTP_H

#include <event2/event.h>
#include <event2/http.h>
#include <event2/buffer.h>
#include <event2/keyvalq_struct.h>
#include "mpack/mpack.h"

// 端点处理函数类型
typedef void (*ya_endpoint_handler_t)(const char* request_data, size_t request_len, mpack_writer_t* writer);

/**
 * @brief 启动HTTP服务
 *
 * @return 0 on success, -1 on failure
 */
int run_http_server(void);

/**
 * @brief 停止HTTP服务
 */
void stop_http_server(void);

#endif // YA_HTTP_H 