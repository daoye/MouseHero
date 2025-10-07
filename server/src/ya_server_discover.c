#include <pthread.h>
#include <stdatomic.h>
#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#ifdef _WIN32
#include <winsock2.h>
#include <ws2tcpip.h> 
#pragma comment(lib, "ws2_32.lib")
#define close closesocket 
#else
#include <arpa/inet.h> 
#include <netinet/in.h>
#include <sys/socket.h>
#include <sys/syslog.h>
#include <sys/types.h>
#include <unistd.h> 
#include <net/if.h>
#include <sys/ioctl.h>
#endif

#include "event2/buffer.h"
#include "ya_event.h"
#include "ya_logger.h"
#include "ya_server.h"
#include "ya_server_discover.h"
#include "ya_utils.h"

#define MAX_PORTS 3
#define DISCOVERY_PORT_START 45101
#define REQUEST_BUFFER_SIZE 512


typedef struct
{
    int sockfd;
    struct sockaddr_in addr;
} ServerInfo;

atomic_int run = 1;
ServerInfo svr = {0};

// // 获取本地IP地址（简化版）
char *get_local_ip(int sockfd)
{
    static char ipstr[INET_ADDRSTRLEN];
    struct sockaddr_in sin;
    socklen_t len = sizeof(sin);

    if (getsockname(sockfd, (struct sockaddr *)&sin, &len) == 0)
    {
        char *addr_str = inet_ntoa(sin.sin_addr);
        if (addr_str) {
            strncpy(ipstr, addr_str, sizeof(ipstr) - 1);
            ipstr[sizeof(ipstr) - 1] = '\0';
            return ipstr;
        }
    }
    return NULL;
}

int create_broadcast_server(int port, ServerInfo *info)
{
    int sockfd;
    struct sockaddr_in serv_addr;
    int broadcast = 1;

    if ((sockfd = socket(AF_INET, SOCK_DGRAM, 0)) < 0)
    {
        YA_LOG_DEBUG("socket creation failed");
        return -1;
    }

    if (setsockopt(sockfd, SOL_SOCKET, SO_BROADCAST, (char *)&broadcast, sizeof(broadcast)) < 0)
    {
        YA_LOG_DEBUG("setsockopt (SO_BROADCAST) failed");
        close(sockfd);
        return -1;
    }

    memset(&serv_addr, 0, sizeof(serv_addr));
    serv_addr.sin_family = AF_INET;
    serv_addr.sin_addr.s_addr = htonl(INADDR_ANY);
    serv_addr.sin_port = htons(port);

    if (bind(sockfd, (struct sockaddr *)&serv_addr, sizeof(serv_addr)) < 0)
    {
        close(sockfd);
        return -1;
    }

    info->sockfd = sockfd;
    memcpy(&info->addr, &serv_addr, sizeof(serv_addr));
    return 0;
}

void *run_loop(void *param)
{
    int port = DISCOVERY_PORT_START;
    create_broadcast_server(DISCOVERY_PORT_START, &svr);

    if (svr.sockfd == 0)
    {
        YA_LOG_DEBUG("Failed to bind any discovery port");
        return NULL;
    }

    YA_LOG_DEBUG("Discovery server listening on port %d", port);

    while (run)
    {
        struct sockaddr_in cli_addr;
        socklen_t addr_len = sizeof(cli_addr);
        uint8_t buffer[REQUEST_BUFFER_SIZE];

        size_t n = recvfrom(svr.sockfd, buffer, REQUEST_BUFFER_SIZE, 0, (struct sockaddr *)&cli_addr, &addr_len);

        struct evbuffer *input = evbuffer_new();
        evbuffer_add(input, buffer, n);

        YAPackageSize size = {0};
        ya_get_package_size(input, &size);
        if (n != (size.totalSize + 4))
        {
            continue;
        }

        // for UDP, remove total size and header size
        evbuffer_drain(input, sizeof(uint32_t) * 2);

        YAEvent requestEvent = {0};
        if (ya_parse_event(input, &requestEvent, &size) < 0)
        {
            continue;
        }

        YADiscoverEventRequest *request = requestEvent.param;

        if (request->magic != DISCOVERY_MAGIC)
        {
            continue;
        }

        YA_LOG_DEBUG("Discovery accepted.");

        YADiscoverEventResponse response = {
            .magic = DISCOVERY_MAGIC,
            .port = svr_context.session_sock_addr.sin_port,
        };

        YAEventHeader responseHeader = {
            .direction = RESPONSE,
            .type = DISCOVER,
            .index = 0,
            .uid = 0,
        };

        YAEvent responseEvent = {.header = responseHeader, .param = &response, .param_len = sizeof(response)};

        uint8_t *rsp = NULL;
        int rspLength = ya_serialize_event(&responseEvent, &rsp);

        sendto(svr.sockfd, rsp, rspLength, 0, (struct sockaddr *)&cli_addr, addr_len);

        safe_free((void **)&rsp);
    }

    return NULL;
}

int run_discovery_server()
{
    run = 1;
    pthread_t thread;
    pthread_create(&thread, NULL, run_loop, NULL);

    return 0;
}

void stop_discovery_server()
{
    run = 0;
    if (svr.sockfd > 0)
    {
        close(svr.sockfd);
    }
}
