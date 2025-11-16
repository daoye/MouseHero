#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <event2/bufferevent.h>
#include "ya_client_manager.h"
#include "ya_logger.h"
#include "ya_mouse_filter.h"

// 计算哈希值
static uint32_t hash_uid(uint32_t uid) {
    return uid % YA_CLIENT_HASH_SIZE;
}

// 添加冲突统计（调试用）
#ifdef DEBUG
static uint32_t collision_count = 0;
static uint32_t max_chain_length = 0;

static void update_collision_stats(ya_client_manager_t *manager) {
    uint32_t max_len = 0;
    uint32_t total_collisions = 0;
    
    for (int i = 0; i < YA_CLIENT_HASH_SIZE; i++) {
        uint32_t chain_len = 0;
        ya_client_t *client = manager->buckets[i];
        while (client) {
            chain_len++;
            client = client->next;
        }
        if (chain_len > 1) {
            total_collisions += (chain_len - 1);
        }
        if (chain_len > max_len) {
            max_len = chain_len;
        }
    }
    
    collision_count = total_collisions;
    max_chain_length = max_len;
    
    if (max_chain_length > 3) {
        YA_LOG_WARN("Hash performance degraded: max_chain_length=%u, collisions=%u", 
                   max_chain_length, collision_count);
    }
}
#endif

void ya_client_manager_init(ya_client_manager_t *manager) {
    if (!manager) return;
    memset(manager, 0, sizeof(ya_client_manager_t));
    manager->next_uid = 1;  // 从1开始分配UID
}

void ya_client_manager_cleanup(ya_client_manager_t *manager) {
    if (!manager) return;
    for (int i = 0; i < YA_CLIENT_HASH_SIZE; i++) {
        ya_client_t *client = manager->buckets[i];
        while (client) {
            ya_client_t *next = client->next;
            if (client->bev) {
                bufferevent_free(client->bev);
            }
            if (client->mouse_filter) {
                ya_mouse_filter_destroy(client->mouse_filter);
                client->mouse_filter = NULL;
            }
            free(client);
            client = next;
        }
        manager->buckets[i] = NULL;
    }
    manager->client_count = 0;
    manager->next_uid = 1;
}

ya_client_t *ya_client_create(ya_client_manager_t *manager, evutil_socket_t fd, struct bufferevent *bev) {
    if (!manager || !bev || fd < 0) {
        YA_LOG_ERROR("Invalid parameters for client creation");
        return NULL;
    }

    ya_client_t *client = (ya_client_t *)calloc(1, sizeof(ya_client_t));
    if (!client) {
        YA_LOG_ERROR("Failed to allocate memory for client");
        return NULL;
    }

    client->fd = fd;
    
    // 处理UID溢出 - 确保不会分配UID 0
    if (manager->next_uid == 0) {
        manager->next_uid = 1;
    }
    client->uid = manager->next_uid++;
    
    // 如果UID即将溢出，回绕到1（避免使用0作为有效UID）
    if (manager->next_uid == 0) {
        manager->next_uid = 1;
        YA_LOG_WARN("UID counter wrapped around");
    }
    
    client->ref_count = 1;  // 初始引用计数为1
    client->state = YA_CLIENT_ACTIVE;
    client->bev = bev;
    client->mouse_filter = ya_mouse_filter_create();
    client->connected_at = time(NULL);
    client->protocol_version = 0;  // 初始化为0，在授权时更新
    if (!client->mouse_filter) {
        YA_LOG_ERROR("Failed to create mouse filter for client %u", client->uid);
        // 不中断创建流程，但记录日志
    }

    // 插入到哈希表
    uint32_t hash = hash_uid(client->uid);
    client->next = manager->buckets[hash];
    manager->buckets[hash] = client;
    manager->client_count++;

    YA_LOG_TRACE("Created new client with UID: %u", client->uid);
    return client;
}

ya_client_t *ya_client_find_by_uid(ya_client_manager_t *manager, uint32_t uid) {
    if (!manager || uid == 0) return NULL;

    uint32_t hash = hash_uid(uid);
    ya_client_t *client = manager->buckets[hash];

    while (client) {
        if (client->uid == uid && client->state == YA_CLIENT_ACTIVE) {
            return client;
        }
        client = client->next;
    }

    return NULL;
}

ya_client_t *ya_client_find_by_fd(ya_client_manager_t *manager, evutil_socket_t fd) {
    if (!manager || fd < 0) return NULL;

    // 由于我们没有为fd建立索引，需要遍历所有bucket
    for (int i = 0; i < YA_CLIENT_HASH_SIZE; i++) {
        ya_client_t *client = manager->buckets[i];
        while (client) {
            if (client->fd == fd && client->state == YA_CLIENT_ACTIVE) {
                return client;
            }
            client = client->next;
        }
    }
    return NULL;
}

void ya_client_ref(ya_client_t *client) {
    if (!client) return;
    client->ref_count++;
    // YA_LOG_TRACE("Client %u ref count increased to %u", client->uid, client->ref_count);
}

void ya_client_unref(ya_client_manager_t *manager, ya_client_t *client) {
    if (!manager || !client) return;

    if (client->ref_count > 0) {
        client->ref_count--;
        // YA_LOG_TRACE("Client %u ref count decreased to %u", client->uid, client->ref_count);

        if (client->ref_count == 0 && client->state == YA_CLIENT_DISCONNECTED) {
            // 找到客户端在哈希表中的位置
            uint32_t hash = hash_uid(client->uid);
            ya_client_t *curr = manager->buckets[hash];
            ya_client_t *prev = NULL;

            while (curr) {
                if (curr == client) {
                    if (prev) {
                        prev->next = curr->next;
                    } else {
                        manager->buckets[hash] = curr->next;
                    }
                    if (client->bev) {
                        bufferevent_free(client->bev);
                    }
                    if (client->mouse_filter) {
                        ya_mouse_filter_destroy(client->mouse_filter);
                        client->mouse_filter = NULL;
                    }
                    free(client);
                    manager->client_count--;
                    YA_LOG_TRACE("Client %u freed", client->uid);
                    break;
                }
                prev = curr;
                curr = curr->next;
            }
        }
    } else {
        YA_LOG_WARN("Attempted to unref client %u with ref_count already at 0", client->uid);
    }
}

void ya_client_remove(ya_client_manager_t *manager, uint32_t uid) {
    if (!manager || uid == 0) return;

    uint32_t hash = hash_uid(uid);
    ya_client_t *client = manager->buckets[hash];
    ya_client_t *prev = NULL;

    while (client) {
        if (client->uid == uid) {
            // 标记为断开连接状态
            client->state = YA_CLIENT_DISCONNECTED;
            YA_LOG_TRACE("Client %u marked as disconnected", uid);

            // 如果没有其他引用，立即释放
            if (client->ref_count == 0) {
                if (prev) {
                    prev->next = client->next;
                } else {
                    manager->buckets[hash] = client->next;
                }
                if (client->bev) {
                    bufferevent_free(client->bev);
                }
                if (client->mouse_filter) {
                    ya_mouse_filter_destroy(client->mouse_filter);
                    client->mouse_filter = NULL;
                }
                free(client);
                manager->client_count--;
                YA_LOG_TRACE("Client %u removed and freed", uid);
            }
            return;
        }
        prev = client;
        client = client->next;
    }
}

uint32_t ya_client_get_count(ya_client_manager_t *manager) {
    if (!manager) return 0;
    
    uint32_t active_count = 0;
    for (int i = 0; i < YA_CLIENT_HASH_SIZE; i++) {
        ya_client_t *client = manager->buckets[i];
        while (client) {
            if (client->state == YA_CLIENT_ACTIVE) {
                active_count++;
            }
            client = client->next;
        }
    }
    return active_count;
} 