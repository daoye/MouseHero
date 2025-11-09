#pragma once

#include <event2/bufferevent.h>

#include "ya_event.h"
#include "ya_client_manager.h"

// 事件处理函数类型
typedef YAEvent* (*ya_event_handler_t)(struct bufferevent *bev, YAEvent *event);

// 辅助函数声明
YAEvent *assign_response(YAEvent *request_event, size_t response_param_len);

// 事件处理函数声明
YAEvent *handle_authorize(struct bufferevent *bev, YAEvent *event);
YAEvent *handle_heartbeat(struct bufferevent *bev, YAEvent *event);
YAEvent *handle_mouse_move(struct bufferevent *bev, YAEvent *event);
YAEvent *handle_mouse_stop(struct bufferevent *bev, YAEvent *event);
YAEvent *handle_mouse_click(struct bufferevent *bev, YAEvent *event);
YAEvent *handle_mouse_scroll(struct bufferevent *bev, YAEvent *event);
YAEvent *handle_keyboard(struct bufferevent *bev, YAEvent *event);
YAEvent *handle_input_text(struct bufferevent *bev, YAEvent *event);
YAEvent *handle_input_get(struct bufferevent *bev, YAEvent *event);
YAEvent *handle_clipboard_get(struct bufferevent *bev, YAEvent *event);
YAEvent *handle_clipboard_set(struct bufferevent *bev, YAEvent *event);
YAEvent *handle_poweroff(struct bufferevent *bev, YAEvent *event);
YAEvent *handle_restart(struct bufferevent *bev, YAEvent *event);
YAEvent *handle_control(struct bufferevent *bev, YAEvent *event);
YAEvent *handle_discover(struct bufferevent *bev, YAEvent *event);

// 主事件处理函数
YAEvent *process_server_event(struct bufferevent *bev, YAEvent *event, ya_client_t *client);
