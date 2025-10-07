// #include <netdb.h>
#include <stdint.h>

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

#include "ya_event.h"
#include "ya_logger.h"
#include "ya_server.h"
#include "ya_server_handler.h"
#include "ya_server_session.h"
#include "ya_utils.h"

static void accept_cb(struct evconnlistener *, evutil_socket_t, struct sockaddr *, int socklen, void *);
static void accept_error_cb(struct evconnlistener *, void *);
static void conn_readcb(struct bufferevent *, void *);
static void conn_eventcb(struct bufferevent *, short, void *);
static void signal_cb(evutil_socket_t, short, void *);

int run_session_server()
{
    struct evconnlistener *listener = evconnlistener_new_bind(
        svr_context.base, accept_cb, NULL, LEV_OPT_REUSEABLE | LEV_OPT_CLOSE_ON_FREE, -1,
        (struct sockaddr *)&svr_context.session_sock_addr, sizeof(svr_context.session_sock_addr));

    if (!listener)
    {
        return -1;
    }

    YA_LOG_DEBUG("Session server listening on %s", svr_context.session_addr);
    return 0;
}

static void accept_cb(struct evconnlistener *listener, evutil_socket_t fd, struct sockaddr *sa, int socklen,
                      void *user_data)
{
    struct event_base *base = evconnlistener_get_base(listener);
    struct bufferevent *bev = bufferevent_socket_new(base, fd, BEV_OPT_CLOSE_ON_FREE);

    if (!bev)
    {
        YA_LOG_DEBUG("Error constructing bufferevent!");
        event_base_loopbreak(base);
        return;
    }

    // 创建新客户端并保存bufferevent
    ya_client_t *client = ya_client_create(&svr_context.client_manager, fd, bev);
    if (!client) {
        YA_LOG_ERROR("Failed to create client");
        bufferevent_free(bev);
        return;
    }

    // 更新连接统计
    ya_server_stats_inc_connections();

    bufferevent_setcb(bev, conn_readcb, NULL, conn_eventcb, client);
    bufferevent_enable(bev, EV_WRITE | EV_READ);
}

static void accept_error_cb(struct evconnlistener *listener, void *ctx)
{
    struct event_base *base = evconnlistener_get_base(listener);
    event_base_loopexit(base, NULL);
    YA_LOG_DEBUG("Session server accept error.");
}

static void conn_readcb(struct bufferevent *bev, void *user_data)
{
    ya_client_t *client = (ya_client_t *)user_data;
    if (!client || client->state != YA_CLIENT_ACTIVE) {
        return;
    }

    struct evbuffer *input = bufferevent_get_input(bev);
    struct evbuffer *output = bufferevent_get_output(bev);

    YAPackageSize size = {0};
    ya_get_package_size(input, &size);

    int len = evbuffer_get_length(input);
    if (len < (size.totalSize + 4))
    {
        return;
    }

    // remove total size and header size
    evbuffer_drain(input, sizeof(uint32_t) * 2);

    YAEvent request = {0};
    if (ya_parse_event(input, &request, &size) < 0)
    {
        return;
    }

    // 增加引用计数
    ya_client_ref(client);

    YAEvent *response = process_server_event(bev, &request, (struct ya_client *)client);

    if (response)
    {
        uint8_t *rsp = NULL;
        int length = ya_serialize_event(response, &rsp);

        if (rsp)
        {
            evbuffer_add(output, rsp, length);
            safe_free((void **)&rsp);
        }
    }

    ya_free_event(response);
    
    // 减少引用计数
    ya_client_unref(&svr_context.client_manager, client);
}

static void conn_eventcb(struct bufferevent *bev, short events, void *user_data)
{
    if (events & (BEV_EVENT_EOF | BEV_EVENT_ERROR))
    {
        ya_client_t *client = (ya_client_t *)user_data;
        if (client)
        {
            // 更新连接统计
            ya_server_stats_dec_connections();
            
            // 标记客户端状态为断开连接
            client->state = YA_CLIENT_DISCONNECTING;
            
            // 清理bufferevent
            if (client->bev) {
                bufferevent_free(client->bev);
                client->bev = NULL;
            }
            
            // 减少引用计数，可能会触发客户端清理
            ya_client_unref(&svr_context.client_manager, client);
        }

        YA_LOG_DEBUG("Connection closed.");
    }
}
