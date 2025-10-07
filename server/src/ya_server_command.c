#include "ya_server_command.h"
#include "ya_event.h"
#include "ya_logger.h"
#include "ya_server.h"
#include "ya_server_handler.h"
#include "ya_utils.h"
#include "ya_client_manager.h"

#include <stdint.h>
#include <unistd.h>

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

static void listener_cb(evutil_socket_t sock, short events, void *user_data);
static void conn_writecb(struct bufferevent *, void *);
static void conn_readcb(struct bufferevent *, void *);
static void conn_eventcb(struct bufferevent *, short, void *);
static void signal_cb(evutil_socket_t, short, void *);
static void udp_readcb(evutil_socket_t sock, short events, void *user_data);

int run_command_server()
{
    svr_context.command_fd = socket(AF_INET, SOCK_DGRAM, 0);
    if (svr_context.command_fd < 0)
    {
        YA_LOG_DEBUG("Can not create socket.");
        return 2;
    }

    if (evutil_make_socket_nonblocking(svr_context.command_fd) < 0)
    {
        return 3;
    }

    if (bind(svr_context.command_fd, (struct sockaddr *)&svr_context.command_sock_addr,
             sizeof(svr_context.command_sock_addr)) < 0)
    {
        YA_LOG_DEBUG("Can't start command server.");
        return 4;
    }

    struct event *event = event_new(svr_context.base, svr_context.command_fd, EV_READ | EV_PERSIST, udp_readcb, NULL);

    if (!event)
    {
        return 5;
    }

    if (event_add(event, NULL) < 0)
    {
        event_free(event);
    }

    YA_LOG_DEBUG("Command server listening on %s", svr_context.command_addr);

    return 0;
}

static void udp_readcb(evutil_socket_t sock, short events, void *user_data)
{
    struct event_base *base = svr_context.base;
    struct sockaddr_in addr;
    socklen_t addr_len = sizeof(addr);
    uint8_t buffer[2048];

    int len = recvfrom(sock, buffer, sizeof(buffer), 0, (struct sockaddr *)&addr, &addr_len);
    if (len < 0)
    {
        YA_LOG_DEBUG("Error receiving data.");
        return;
    }

    struct evbuffer *input = evbuffer_new();
    evbuffer_add(input, buffer, len);

    YAPackageSize size = {0};
    ya_get_package_size(input, &size);
    if (len != (size.totalSize + 4))
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

    ya_client_t *client = get_client_by_id(request.header.uid);
    YAEvent *response = process_server_event(NULL, &request, client);

    if (NULL != response)
    {
        uint8_t *rsp = NULL;
        int length = ya_serialize_event(response, &rsp);
        sendto(sock, rsp, length, 0, (struct sockaddr *)&addr, addr_len);

        safe_free((void **)&rsp);
    }

    ya_free_event(response);
    evbuffer_free(input);
}