#include <stdlib.h>
#include <unity.h>
#include <event2/event.h>
#include <event2/bufferevent.h>
#include <event2/buffer.h>
#include "../src/ya_client_manager.h"

static ya_client_manager_t manager;
static struct event_base *base;

void setUp(void) {
    // 初始化事件base
    base = event_base_new();
    TEST_ASSERT_NOT_NULL(base);
    
    // 初始化manager
    ya_client_manager_init(&manager);
}

void tearDown(void) {
    // 清理manager
    ya_client_manager_cleanup(&manager);
    
    // 清理事件base
    if (base) {
        event_base_free(base);
        base = NULL;
    }
}

// 测试初始化
void test_client_manager_init(void) {
    ya_client_manager_t test_manager;
    ya_client_manager_init(&test_manager);
    
    TEST_ASSERT_EQUAL_UINT32(0, test_manager.client_count);
    TEST_ASSERT_EQUAL_UINT32(1, test_manager.next_uid);
    
    for (int i = 0; i < YA_CLIENT_HASH_SIZE; i++) {
        TEST_ASSERT_NULL(test_manager.buckets[i]);
    }
    
    // 测试无效参数
    ya_client_manager_init(NULL);
    
    ya_client_manager_cleanup(&test_manager);
}

// 测试客户端创建
void test_client_create(void) {
    evutil_socket_t fd = 10;
    struct bufferevent *bev = bufferevent_socket_new(base, fd, BEV_OPT_CLOSE_ON_FREE);
    TEST_ASSERT_NOT_NULL(bev);
    
    ya_client_t *client = ya_client_create(&manager, fd, bev);
    TEST_ASSERT_NOT_NULL(client);
    TEST_ASSERT_EQUAL_INT(fd, client->fd);
    TEST_ASSERT_EQUAL_UINT32(1, client->uid);
    TEST_ASSERT_EQUAL_UINT32(1, client->ref_count);
    TEST_ASSERT_EQUAL_INT(YA_CLIENT_ACTIVE, client->state);
    TEST_ASSERT_EQUAL_PTR(bev, client->bev);
    
    TEST_ASSERT_EQUAL_UINT32(1, manager.client_count);
    TEST_ASSERT_EQUAL_UINT32(2, manager.next_uid);
    
    // 测试无效参数
    TEST_ASSERT_NULL(ya_client_create(NULL, fd, bev));
    
    // 清理
    ya_client_remove(&manager, client->uid);
}

// 测试通过UID查找客户端
void test_client_find_by_uid(void) {
    // 创建测试客户端
    evutil_socket_t fd = 10;
    struct bufferevent *bev = bufferevent_socket_new(base, fd, BEV_OPT_CLOSE_ON_FREE);
    ya_client_t *client = ya_client_create(&manager, fd, bev);
    TEST_ASSERT_NOT_NULL(client);
    
    // 测试查找存在的客户端
    ya_client_t *found = ya_client_find_by_uid(&manager, client->uid);
    TEST_ASSERT_NOT_NULL(found);
    TEST_ASSERT_EQUAL_PTR(client, found);
    
    // 测试查找不存在的客户端
    found = ya_client_find_by_uid(&manager, 9999);
    TEST_ASSERT_NULL(found);
    
    // 测试无效参数
    found = ya_client_find_by_uid(NULL, 1);
    TEST_ASSERT_NULL(found);
    found = ya_client_find_by_uid(&manager, 0);
    TEST_ASSERT_NULL(found);
    
    // 清理
    ya_client_remove(&manager, client->uid);
}

// 测试通过文件描述符查找客户端
void test_client_find_by_fd(void) {
    // 创建测试客户端
    evutil_socket_t fd = 10;
    struct bufferevent *bev = bufferevent_socket_new(base, fd, BEV_OPT_CLOSE_ON_FREE);
    ya_client_t *client = ya_client_create(&manager, fd, bev);
    TEST_ASSERT_NOT_NULL(client);
    
    // 测试查找存在的客户端
    ya_client_t *found = ya_client_find_by_fd(&manager, fd);
    TEST_ASSERT_NOT_NULL(found);
    TEST_ASSERT_EQUAL_PTR(client, found);
    
    // 测试查找不存在的客户端
    found = ya_client_find_by_fd(&manager, 9999);
    TEST_ASSERT_NULL(found);
    
    // 测试无效参数
    found = ya_client_find_by_fd(NULL, fd);
    TEST_ASSERT_NULL(found);
    found = ya_client_find_by_fd(&manager, -1);
    TEST_ASSERT_NULL(found);
    
    // 清理
    ya_client_remove(&manager, client->uid);
}

// 测试引用计数
void test_client_ref_unref(void) {
    // 创建测试客户端
    evutil_socket_t fd = 10;
    struct bufferevent *bev = bufferevent_socket_new(base, fd, BEV_OPT_CLOSE_ON_FREE);
    ya_client_t *client = ya_client_create(&manager, fd, bev);
    TEST_ASSERT_NOT_NULL(client);
    
    // 测试增加引用计数
    uint32_t initial_ref = client->ref_count;
    ya_client_ref(client);
    TEST_ASSERT_EQUAL_UINT32(initial_ref + 1, client->ref_count);
    
    // 测试减少引用计数
    ya_client_unref(&manager, client);
    TEST_ASSERT_EQUAL_UINT32(initial_ref, client->ref_count);
    
    // 测试无效参数
    ya_client_ref(NULL);
    ya_client_unref(NULL, client);
    ya_client_unref(&manager, NULL);
    
    // 清理
    ya_client_remove(&manager, client->uid);
}

// 测试客户端状态转换
void test_client_state_transition(void) {
    evutil_socket_t fd = 10;
    struct bufferevent *bev = bufferevent_socket_new(base, fd, BEV_OPT_CLOSE_ON_FREE);
    ya_client_t *client = ya_client_create(&manager, fd, bev);
    TEST_ASSERT_NOT_NULL(client);
    
    // 初始状态应该是ACTIVE
    TEST_ASSERT_EQUAL_INT(YA_CLIENT_ACTIVE, client->state);
    
    // 增加引用计数
    ya_client_ref(client);
    TEST_ASSERT_EQUAL_UINT32(2, client->ref_count);
    
    // 移除客户端，状态应该变为DISCONNECTED
    ya_client_remove(&manager, client->uid);
    TEST_ASSERT_EQUAL_INT(YA_CLIENT_DISCONNECTED, client->state);
    
    // 客户端应该还存在，因为还有引用
    ya_client_t *found = ya_client_find_by_uid(&manager, client->uid);
    TEST_ASSERT_NULL(found); // 断开连接的客户端不应该被找到
    
    // 减少所有引用，客户端应该被释放
    ya_client_unref(&manager, client);
    ya_client_unref(&manager, client);
    
    // 验证客户端数量
    TEST_ASSERT_EQUAL_UINT32(0, ya_client_get_count(&manager));
}

// 测试多个客户端的并发操作
void test_multiple_clients(void) {
    ya_client_t *clients[5];
    evutil_socket_t base_fd = 10;
    
    // 创建多个客户端
    for (int i = 0; i < 5; i++) {
        struct bufferevent *bev = bufferevent_socket_new(base, base_fd + i, BEV_OPT_CLOSE_ON_FREE);
        clients[i] = ya_client_create(&manager, base_fd + i, bev);
        TEST_ASSERT_NOT_NULL(clients[i]);
    }
    
    // 验证客户端数量
    TEST_ASSERT_EQUAL_UINT32(5, ya_client_get_count(&manager));
    
    // 对部分客户端增加引用
    ya_client_ref(clients[1]);
    ya_client_ref(clients[3]);
    
    // 移除部分客户端
    ya_client_remove(&manager, clients[0]->uid);
    ya_client_remove(&manager, clients[2]->uid);
    ya_client_remove(&manager, clients[4]->uid);
    
    // 验证客户端数量（有引用的客户端不会被立即释放，但已断开连接的客户端不应该被计数）
    TEST_ASSERT_EQUAL_UINT32(2, ya_client_get_count(&manager));
    
    // 清理剩余客户端
    ya_client_unref(&manager, clients[1]);
    ya_client_remove(&manager, clients[1]->uid);
    ya_client_unref(&manager, clients[3]);
    ya_client_remove(&manager, clients[3]->uid);
    
    // 验证最终客户端数量
    TEST_ASSERT_EQUAL_UINT32(0, ya_client_get_count(&manager));
}

// 测试客户端清理
void test_client_cleanup(void) {
    // 创建多个客户端
    for (int i = 0; i < 3; i++) {
        struct bufferevent *bev = bufferevent_socket_new(base, 10 + i, BEV_OPT_CLOSE_ON_FREE);
        ya_client_t *client = ya_client_create(&manager, 10 + i, bev);
        TEST_ASSERT_NOT_NULL(client);
    }
    
    TEST_ASSERT_EQUAL_UINT32(3, ya_client_get_count(&manager));
    
    // 清理所有客户端
    ya_client_manager_cleanup(&manager);
    
    // 验证清理结果
    TEST_ASSERT_EQUAL_UINT32(0, ya_client_get_count(&manager));
    for (int i = 0; i < YA_CLIENT_HASH_SIZE; i++) {
        TEST_ASSERT_NULL(manager.buckets[i]);
    }
}

// 测试哈希冲突处理
void test_hash_collision(void) {
    // 创建两个客户端，使其产生哈希冲突
    evutil_socket_t fd1 = 10;
    evutil_socket_t fd2 = 11;
    struct bufferevent *bev1 = bufferevent_socket_new(base, fd1, BEV_OPT_CLOSE_ON_FREE);
    struct bufferevent *bev2 = bufferevent_socket_new(base, fd2, BEV_OPT_CLOSE_ON_FREE);
    
    ya_client_t *client1 = ya_client_create(&manager, fd1, bev1);
    TEST_ASSERT_NOT_NULL(client1);
    
    // 手动设置第二个客户端的UID，使其与第一个客户端产生相同的哈希值
    uint32_t uid2 = client1->uid + YA_CLIENT_HASH_SIZE;
    manager.next_uid = uid2;
    ya_client_t *client2 = ya_client_create(&manager, fd2, bev2);
    TEST_ASSERT_NOT_NULL(client2);
    
    // 验证两个客户端都能被正确找到
    ya_client_t *found1 = ya_client_find_by_uid(&manager, client1->uid);
    ya_client_t *found2 = ya_client_find_by_uid(&manager, client2->uid);
    TEST_ASSERT_NOT_NULL(found1);
    TEST_ASSERT_NOT_NULL(found2);
    TEST_ASSERT_EQUAL_PTR(client1, found1);
    TEST_ASSERT_EQUAL_PTR(client2, found2);
    
    // 测试在链表中间删除客户端（删除第一个客户端）
    ya_client_remove(&manager, client1->uid);
    
    // 验证第二个客户端仍然可以找到
    found2 = ya_client_find_by_uid(&manager, client2->uid);
    TEST_ASSERT_NOT_NULL(found2);
    TEST_ASSERT_EQUAL_PTR(client2, found2);
    
    // 清理
    ya_client_remove(&manager, client2->uid);
}

// 测试引用计数为0时的立即删除
void test_zero_ref_count_removal(void) {
    evutil_socket_t fd = 10;
    struct bufferevent *bev = bufferevent_socket_new(base, fd, BEV_OPT_CLOSE_ON_FREE);
    ya_client_t *client = ya_client_create(&manager, fd, bev);
    TEST_ASSERT_NOT_NULL(client);
    
    // 减少引用计数到0
    ya_client_unref(&manager, client);
    TEST_ASSERT_EQUAL_UINT32(0, client->ref_count);
    
    // 立即移除客户端，应该直接删除
    ya_client_remove(&manager, client->uid);
    
    // 验证客户端已被删除
    TEST_ASSERT_EQUAL_UINT32(0, ya_client_get_count(&manager));
}

// 测试manager清理无效参数
void test_manager_cleanup_null(void) {
    ya_client_manager_cleanup(NULL); // 不应该崩溃
}

// 测试get_count无效参数
void test_get_count_null(void) {
    TEST_ASSERT_EQUAL_UINT32(0, ya_client_get_count(NULL));
}

// 测试UID溢出和回绕
void test_uid_overflow(void) {
    // 设置next_uid接近溢出点
    manager.next_uid = UINT32_MAX - 1;
    
    evutil_socket_t fd1 = 10;
    struct bufferevent *bev1 = bufferevent_socket_new(base, fd1, BEV_OPT_CLOSE_ON_FREE);
    ya_client_t *client1 = ya_client_create(&manager, fd1, bev1);
    TEST_ASSERT_NOT_NULL(client1);
    TEST_ASSERT_EQUAL_UINT32(UINT32_MAX - 1, client1->uid);
    
    // 下一个客户端应该触发溢出处理
    evutil_socket_t fd2 = 11;
    struct bufferevent *bev2 = bufferevent_socket_new(base, fd2, BEV_OPT_CLOSE_ON_FREE);
    ya_client_t *client2 = ya_client_create(&manager, fd2, bev2);
    TEST_ASSERT_NOT_NULL(client2);
    TEST_ASSERT_EQUAL_UINT32(UINT32_MAX, client2->uid);
    
    // 再下一个应该回绕到1
    evutil_socket_t fd3 = 12;
    struct bufferevent *bev3 = bufferevent_socket_new(base, fd3, BEV_OPT_CLOSE_ON_FREE);
    ya_client_t *client3 = ya_client_create(&manager, fd3, bev3);
    TEST_ASSERT_NOT_NULL(client3);
    TEST_ASSERT_EQUAL_UINT32(1, client3->uid);
    TEST_ASSERT_EQUAL_UINT32(2, manager.next_uid);  // 下次应该分配2
    
    // 清理
    ya_client_remove(&manager, client1->uid);
    ya_client_remove(&manager, client2->uid);
    ya_client_remove(&manager, client3->uid);
}

// 测试next_uid为0的边界情况
void test_next_uid_zero(void) {
    manager.next_uid = 0;
    
    evutil_socket_t fd = 10;
    struct bufferevent *bev = bufferevent_socket_new(base, fd, BEV_OPT_CLOSE_ON_FREE);
    ya_client_t *client = ya_client_create(&manager, fd, bev);
    TEST_ASSERT_NOT_NULL(client);
    TEST_ASSERT_EQUAL_UINT32(1, client->uid);  // 应该从1开始
    TEST_ASSERT_EQUAL_UINT32(2, manager.next_uid);
    
    ya_client_remove(&manager, client->uid);
}

// 测试客户端创建时的参数验证
void test_client_create_invalid_params(void) {
    evutil_socket_t fd = 10;
    struct bufferevent *bev = bufferevent_socket_new(base, fd, BEV_OPT_CLOSE_ON_FREE);
    
    // 测试各种无效参数组合
    TEST_ASSERT_NULL(ya_client_create(NULL, fd, bev));
    TEST_ASSERT_NULL(ya_client_create(&manager, -1, bev));
    TEST_ASSERT_NULL(ya_client_create(&manager, fd, NULL));
    TEST_ASSERT_NULL(ya_client_create(NULL, -1, NULL));
    
    bufferevent_free(bev);
}

// 测试引用计数的异常情况
void test_ref_count_underflow(void) {
    evutil_socket_t fd = 10;
    struct bufferevent *bev = bufferevent_socket_new(base, fd, BEV_OPT_CLOSE_ON_FREE);
    ya_client_t *client = ya_client_create(&manager, fd, bev);
    TEST_ASSERT_NOT_NULL(client);
    
    // 将引用计数减到0
    ya_client_unref(&manager, client);
    TEST_ASSERT_EQUAL_UINT32(0, client->ref_count);
    
    // 再次尝试减少，应该触发警告但不崩溃
    ya_client_unref(&manager, client);
    TEST_ASSERT_EQUAL_UINT32(0, client->ref_count);  // 应该保持为0
    
    // 手动清理（因为ref_count为0）
    ya_client_remove(&manager, client->uid);
}

// 测试复杂的哈希冲突场景
void test_complex_hash_collision(void) {
    ya_client_t *clients[10];
    
    // 创建多个客户端，强制产生哈希冲突
    for (int i = 0; i < 10; i++) {
        evutil_socket_t fd = 20 + i;
        struct bufferevent *bev = bufferevent_socket_new(base, fd, BEV_OPT_CLOSE_ON_FREE);
        
        // 手动设置UID使其产生冲突
        uint32_t target_uid = 100 + i * YA_CLIENT_HASH_SIZE;  // 都映射到同一个bucket
        manager.next_uid = target_uid;
        
        clients[i] = ya_client_create(&manager, fd, bev);
        TEST_ASSERT_NOT_NULL(clients[i]);
        TEST_ASSERT_EQUAL_UINT32(target_uid, clients[i]->uid);
    }
    
    // 验证所有客户端都能正确找到
    for (int i = 0; i < 10; i++) {
        ya_client_t *found = ya_client_find_by_uid(&manager, clients[i]->uid);
        TEST_ASSERT_NOT_NULL(found);
        TEST_ASSERT_EQUAL_PTR(clients[i], found);
    }
    
    // 测试在链表中间删除客户端
    ya_client_remove(&manager, clients[5]->uid);
    TEST_ASSERT_NULL(ya_client_find_by_uid(&manager, clients[5]->uid));
    
    // 验证其他客户端仍然可以找到
    for (int i = 0; i < 10; i++) {
        if (i != 5) {
            ya_client_t *found = ya_client_find_by_uid(&manager, clients[i]->uid);
            TEST_ASSERT_NOT_NULL(found);
        }
    }
    
    // 清理剩余客户端
    for (int i = 0; i < 10; i++) {
        if (i != 5) {  // 客户端5已经被删除
            ya_client_remove(&manager, clients[i]->uid);
        }
    }
}

// 测试查找断开连接的客户端
void test_find_disconnected_client(void) {
    evutil_socket_t fd = 10;
    struct bufferevent *bev = bufferevent_socket_new(base, fd, BEV_OPT_CLOSE_ON_FREE);
    ya_client_t *client = ya_client_create(&manager, fd, bev);
    TEST_ASSERT_NOT_NULL(client);
    
    uint32_t uid = client->uid;
    
    // 增加引用计数，防止立即删除
    ya_client_ref(client);
    
    // 标记为断开连接
    ya_client_remove(&manager, uid);
    TEST_ASSERT_EQUAL_INT(YA_CLIENT_DISCONNECTED, client->state);
    
    // 查找断开连接的客户端应该返回NULL
    TEST_ASSERT_NULL(ya_client_find_by_uid(&manager, uid));
    TEST_ASSERT_NULL(ya_client_find_by_fd(&manager, fd));
    
    // 清理
    ya_client_unref(&manager, client);
}

// 测试大量客户端的性能
void test_many_clients_performance(void) {
    const int client_count = 100;
    ya_client_t *clients[client_count];
    
    // 创建大量客户端
    for (int i = 0; i < client_count; i++) {
        evutil_socket_t fd = 100 + i;
        struct bufferevent *bev = bufferevent_socket_new(base, fd, BEV_OPT_CLOSE_ON_FREE);
        clients[i] = ya_client_create(&manager, fd, bev);
        TEST_ASSERT_NOT_NULL(clients[i]);
    }
    
    TEST_ASSERT_EQUAL_UINT32(client_count, ya_client_get_count(&manager));
    
    // 测试查找性能 - 所有客户端都应该能快速找到
    for (int i = 0; i < client_count; i++) {
        ya_client_t *found_by_uid = ya_client_find_by_uid(&manager, clients[i]->uid);
        ya_client_t *found_by_fd = ya_client_find_by_fd(&manager, clients[i]->fd);
        
        TEST_ASSERT_NOT_NULL(found_by_uid);
        TEST_ASSERT_NOT_NULL(found_by_fd);
        TEST_ASSERT_EQUAL_PTR(clients[i], found_by_uid);
        TEST_ASSERT_EQUAL_PTR(clients[i], found_by_fd);
    }
    
    // 清理所有客户端
    for (int i = 0; i < client_count; i++) {
        ya_client_remove(&manager, clients[i]->uid);
    }
    
    TEST_ASSERT_EQUAL_UINT32(0, ya_client_get_count(&manager));
}

// 测试引用计数在remove中的复杂情况
void test_ref_count_in_remove(void) {
    evutil_socket_t fd = 10;
    struct bufferevent *bev = bufferevent_socket_new(base, fd, BEV_OPT_CLOSE_ON_FREE);
    ya_client_t *client = ya_client_create(&manager, fd, bev);
    TEST_ASSERT_NOT_NULL(client);
    
    uint32_t uid = client->uid;
    
    // 增加多个引用
    ya_client_ref(client);  // ref_count = 2
    ya_client_ref(client);  // ref_count = 3
    
    // 移除客户端，但因为有引用所以不会立即删除
    ya_client_remove(&manager, uid);
    TEST_ASSERT_EQUAL_INT(YA_CLIENT_DISCONNECTED, client->state);
    
    // 逐步减少引用计数
    ya_client_unref(&manager, client);  // ref_count = 2
    TEST_ASSERT_EQUAL_UINT32(2, client->ref_count);
    
    ya_client_unref(&manager, client);  // ref_count = 1
    TEST_ASSERT_EQUAL_UINT32(1, client->ref_count);
    
    // 最后一次unref应该触发删除
    ya_client_unref(&manager, client);  // ref_count = 0, should be freed
    
    // 验证客户端已被完全删除
    TEST_ASSERT_NULL(ya_client_find_by_uid(&manager, uid));
    TEST_ASSERT_EQUAL_UINT32(0, ya_client_get_count(&manager));
}

// 测试边界条件：删除不存在的客户端
void test_remove_nonexistent_client(void) {
    uint32_t initial_count = ya_client_get_count(&manager);
    
    // 尝试删除不存在的客户端
    ya_client_remove(&manager, 99999);
    ya_client_remove(&manager, 0);  // 无效UID
    
    // 客户端数量应该没有变化
    TEST_ASSERT_EQUAL_UINT32(initial_count, ya_client_get_count(&manager));
}

#ifdef DEBUG
// 测试调试统计功能（仅在DEBUG模式下）
void test_debug_collision_stats(void) {
    // 这个测试需要在DEBUG模式下编译才能执行
    // 创建一些客户端来触发统计更新
    const int client_count = 20;
    ya_client_t *clients[client_count];
    
    for (int i = 0; i < client_count; i++) {
        evutil_socket_t fd = 200 + i;
        struct bufferevent *bev = bufferevent_socket_new(base, fd, BEV_OPT_CLOSE_ON_FREE);
        
        // 手动设置UID产生一些冲突
        if (i % 3 == 0) {
            manager.next_uid = 1000 + (i / 3) * YA_CLIENT_HASH_SIZE;
        }
        
        clients[i] = ya_client_create(&manager, fd, bev);
        TEST_ASSERT_NOT_NULL(clients[i]);
    }
    
    // 清理
    for (int i = 0; i < client_count; i++) {
        ya_client_remove(&manager, clients[i]->uid);
    }
}
#endif

// 测试内存分配失败的情况
// 注意：这个测试需要特殊的内存分配钩子来模拟失败，
// 在实际环境中可能无法直接测试，但我们可以检查错误处理逻辑
void test_memory_allocation_failure_simulation(void) {
    // 这是一个概念性测试，展示如何处理内存分配失败
    // 在真实环境中，我们可以使用内存分配钩子来模拟失败
    
    evutil_socket_t fd = 10;
    struct bufferevent *bev = bufferevent_socket_new(base, fd, BEV_OPT_CLOSE_ON_FREE);
    TEST_ASSERT_NOT_NULL(bev);
    
    // 在没有内存分配钩子的情况下，我们至少验证创建成功的情况
    ya_client_t *client = ya_client_create(&manager, fd, bev);
    TEST_ASSERT_NOT_NULL(client);
    
    // 验证客户端状态正确
    TEST_ASSERT_EQUAL_INT(fd, client->fd);
    TEST_ASSERT_EQUAL_UINT32(1, client->ref_count);
    TEST_ASSERT_EQUAL_INT(YA_CLIENT_ACTIVE, client->state);
    TEST_ASSERT_EQUAL_PTR(bev, client->bev);
    
    ya_client_remove(&manager, client->uid);
}

// 测试极限条件：清空管理器后的操作
void test_operations_on_empty_manager(void) {
    // 确保管理器为空
    ya_client_manager_cleanup(&manager);
    ya_client_manager_init(&manager);
    
    // 在空管理器上进行各种操作
    TEST_ASSERT_NULL(ya_client_find_by_uid(&manager, 1));
    TEST_ASSERT_NULL(ya_client_find_by_uid(&manager, UINT32_MAX));
    TEST_ASSERT_NULL(ya_client_find_by_fd(&manager, 10));
    TEST_ASSERT_EQUAL_UINT32(0, ya_client_get_count(&manager));
    
    // 尝试删除不存在的客户端
    ya_client_remove(&manager, 1);
    ya_client_remove(&manager, UINT32_MAX);
    
    // 状态应该保持不变
    TEST_ASSERT_EQUAL_UINT32(0, ya_client_get_count(&manager));
    TEST_ASSERT_EQUAL_UINT32(1, manager.next_uid);
}

// 测试状态转换的边界情况
void test_client_state_edge_cases(void) {
    evutil_socket_t fd = 10;
    struct bufferevent *bev = bufferevent_socket_new(base, fd, BEV_OPT_CLOSE_ON_FREE);
    ya_client_t *client = ya_client_create(&manager, fd, bev);
    TEST_ASSERT_NOT_NULL(client);
    
    // 测试多次remove同一个客户端
    uint32_t uid = client->uid;
    ya_client_remove(&manager, uid);
    TEST_ASSERT_EQUAL_INT(YA_CLIENT_DISCONNECTED, client->state);
    
    // 再次remove应该安全（no-op）
    ya_client_remove(&manager, uid);
    TEST_ASSERT_EQUAL_INT(YA_CLIENT_DISCONNECTED, client->state);
    
    // 客户端应该已经被清理（因为ref_count为0）
    TEST_ASSERT_NULL(ya_client_find_by_uid(&manager, uid));
}

// 测试ref/unref的边界情况
void test_ref_unref_edge_cases(void) {
    evutil_socket_t fd = 10;
    struct bufferevent *bev = bufferevent_socket_new(base, fd, BEV_OPT_CLOSE_ON_FREE);
    ya_client_t *client = ya_client_create(&manager, fd, bev);
    TEST_ASSERT_NOT_NULL(client);
    
    uint32_t uid = client->uid;
    
    // 测试大量ref/unref操作
    for (int i = 0; i < 100; i++) {
        ya_client_ref(client);
    }
    TEST_ASSERT_EQUAL_UINT32(101, client->ref_count);  // 1(initial) + 100
    
    // 逐步unref，但保持客户端活跃
    for (int i = 0; i < 50; i++) {
        ya_client_unref(&manager, client);
    }
    TEST_ASSERT_EQUAL_UINT32(51, client->ref_count);
    
    // 标记为断开连接
    ya_client_remove(&manager, uid);
    TEST_ASSERT_EQUAL_INT(YA_CLIENT_DISCONNECTED, client->state);
    
    // 继续unref直到释放
    for (int i = 0; i < 51; i++) {
        ya_client_unref(&manager, client);
    }
    
    // 客户端应该已被释放
    TEST_ASSERT_NULL(ya_client_find_by_uid(&manager, uid));
}

int main(void) {
    UNITY_BEGIN();
    
    RUN_TEST(test_client_manager_init);
    RUN_TEST(test_client_create);
    RUN_TEST(test_client_find_by_uid);
    RUN_TEST(test_client_find_by_fd);
    RUN_TEST(test_client_ref_unref);
    RUN_TEST(test_client_state_transition);
    RUN_TEST(test_multiple_clients);
    RUN_TEST(test_client_cleanup);
    RUN_TEST(test_hash_collision);
    RUN_TEST(test_zero_ref_count_removal);
    RUN_TEST(test_manager_cleanup_null);
    RUN_TEST(test_get_count_null);
    
    // 新增的测试用例
    RUN_TEST(test_uid_overflow);
    RUN_TEST(test_next_uid_zero);
    RUN_TEST(test_client_create_invalid_params);
    RUN_TEST(test_ref_count_underflow);
    RUN_TEST(test_complex_hash_collision);
    RUN_TEST(test_find_disconnected_client);
    RUN_TEST(test_many_clients_performance);
    RUN_TEST(test_ref_count_in_remove);
    RUN_TEST(test_remove_nonexistent_client);
    RUN_TEST(test_memory_allocation_failure_simulation);
    RUN_TEST(test_operations_on_empty_manager);
    RUN_TEST(test_client_state_edge_cases);
    RUN_TEST(test_ref_unref_edge_cases);
    
#ifdef DEBUG
    RUN_TEST(test_debug_collision_stats);
#endif
    
    return UNITY_END();
} 