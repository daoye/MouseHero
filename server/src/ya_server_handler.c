#include <limits.h>
#include <stdbool.h>
#include <stddef.h>
#include <stdlib.h>
#include <string.h>

#include "key_inject.h"
#include "keycode_map.h"
#include "rs.h"
#include "ya_authorize.h"
#include "ya_client_manager.h"
#include "ya_event.h"
#include "ya_logger.h"
#include "ya_mouse_filter.h"
#include "ya_utils.h"
#include "ya_mouse_throttle.h"
#include "ya_server.h"
#include "ya_server_handler.h"
#include "ya_power_scripts.h"

 

extern YA_ServerContext svr_context;

 

#if defined(_WIN32) || defined(_WIN64)
#  include <windows.h>
static CRITICAL_SECTION g_key_inject_cs;
static INIT_ONCE g_key_inject_once = INIT_ONCE_STATIC_INIT;
static BOOL CALLBACK ya_init_key_cs(PINIT_ONCE initOnce, PVOID param, PVOID *context)
{
    (void)initOnce; (void)param; (void)context;
    InitializeCriticalSection(&g_key_inject_cs);
    return TRUE;
}
static inline void ya_key_lock(void)
{
    InitOnceExecuteOnce(&g_key_inject_once, ya_init_key_cs, NULL, NULL);
    EnterCriticalSection(&g_key_inject_cs);
}
static inline void ya_key_unlock(void)
{
    LeaveCriticalSection(&g_key_inject_cs);
}
static inline void ya_key_sleep_ms(unsigned ms)
{
    Sleep(ms);
}
#else
#  include <pthread.h>
#  include <unistd.h>
static pthread_mutex_t g_key_inject_mutex = PTHREAD_MUTEX_INITIALIZER;
static inline void ya_key_lock(void)
{
    (void)pthread_mutex_lock(&g_key_inject_mutex);
}
static inline void ya_key_unlock(void)
{
    (void)pthread_mutex_unlock(&g_key_inject_mutex);
}
static inline void ya_key_sleep_ms(unsigned ms)
{
    usleep(ms * 1000);
}
#endif

// 每步按键的最小间隔（毫秒），用于组合键之间留出有效间隔
static const unsigned YA_KEY_DELAY_MS = 12; // 10~20ms 经验值

YAEvent *assign_response(YAEvent *request_event, size_t response_param_len)
{
    YAEvent *resposne_event = malloc(sizeof(YAEvent));
    if (!resposne_event)
    {
        YA_LOG_ERROR("Failed to allocate response event");
        return NULL;
    }

    memset(resposne_event, 0, sizeof(YAEvent));
    resposne_event->header.direction = RESPONSE;
    resposne_event->header.uid = request_event->header.uid;
    resposne_event->header.index = request_event->header.index;
    resposne_event->header.type = request_event->header.type;

    if (response_param_len > 0)
    {
        resposne_event->param_len = response_param_len;
        resposne_event->param = malloc(response_param_len);
        if (!resposne_event->param)
        {
            YA_LOG_ERROR("Failed to allocate response parameters");
            free(resposne_event);
            return NULL;
        }
        memset(resposne_event->param, 0, response_param_len);
    }

    return resposne_event;
}

YAEvent *handle_authorize(struct bufferevent *bev, YAEvent *event)
{
    if (!event || !event->param)
    {
        YA_LOG_ERROR("Invalid authorize event or parameters");
        return NULL;
    }

    YAEvent *response_event = assign_response(event, sizeof(YAAuthorizeEventResponse));
    if (!response_event)
    {
        YA_LOG_ERROR("Failed to allocate response event");
        return NULL;
    }

    YAAuthorizeEventResponse *response = response_event->param;
    if (!response)
    {
        YA_LOG_ERROR("Failed to allocate response parameters");
        ya_free_event(response_event);
        return NULL;
    }

    response->success = authorize(event->param);
    response->session_port = 0;
    response->command_port = 0;
    response->macs = NULL;
    // 填充服务端 OS 类型
    #if defined(_WIN32) || defined(_WIN64)
        response->os_type = OS_WINDOWS;
    #elif defined(__APPLE__)
        response->os_type = OS_MAC;
    #elif defined(__linux__)
        response->os_type = OS_LINUX;
    #else
        response->os_type = 0; // 未知
    #endif

    if (response->success)
    {
        evutil_socket_t fd = bufferevent_getfd(bev);
        ya_client_t *client = ya_client_find_by_fd(&svr_context.client_manager, fd);
        if (!client)
        {
            YA_LOG_ERROR("Authorize failed: no client bound to fd=%d, closing connection", (int)fd);
            ya_free_event(response_event);
            bufferevent_free(bev); 
            return NULL;
        }

        client->bev = bev; // The connection owns the bufferevent
        response->uid = client->uid;

        // 填充端口（主机序）
        response->session_port = ntohs(svr_context.session_sock_addr.sin_port);
        response->command_port = ntohs(svr_context.command_sock_addr.sin_port);
        // 复制缓存的MAC字符串
        if (svr_context.macs_csv && svr_context.macs_csv[0] != '\0')
        {
            response->macs = strdup(svr_context.macs_csv);
        }
        else
        {
            response->macs = strdup("");
        }

        response->display_scale = ya_primary_scale();

        if (client && client->mouse_filter) {
            ya_mouse_filter_set_algorithms_enabled(client->mouse_filter, true);
        }
    }
    else
    {
        YA_LOG_ERROR("Authorization failed: %s", get_authorize_error());
        response->uid = 0;
        response->session_port = 0;
        response->command_port = 0;
        response->macs = strdup("");
        response->display_scale = 1.0f;
    }

    return response_event;
}

YAEvent *handle_session_option(struct bufferevent *bev, YAEvent *event)
{
    (void)bev;
    if (!event || !event->param)
    {
        YA_LOG_WARN("SESSION_OPTION event missing parameters");
        return NULL;
    }

    ya_client_t *client = ya_client_find_by_uid(&svr_context.client_manager, event->header.uid);
    if (!client)
    {
        YA_LOG_WARN("SESSION_OPTION: client not found for uid=%u", event->header.uid);
        return NULL;
    }

    YASessionOptionEventRequest *req = (YASessionOptionEventRequest *)event->param;
    if (client->mouse_filter)
    {
        float scale = req->pointer_scale;
        if (scale <= 0.0f)
        {
            scale = 1.0f;
        }
        ya_mouse_filter_set_pointer_scale(client->mouse_filter, scale);
        YA_LOG_DEBUG("SESSION_OPTION set pointer_scale=%.3f for client %u", scale, client->uid);
    }

    return NULL;
}

YAEvent *handle_heartbeat(struct bufferevent *bev, YAEvent *event)
{
    if (!event)
    {
        YA_LOG_ERROR("Invalid heartbeat event");
        return NULL;
    }

    YAEvent *response = assign_response(event, 0);
    if (!response)
    {
        YA_LOG_ERROR("Failed to allocate response event");
        return NULL;
    }

    return response;
}

YAEvent *handle_mouse_move(struct bufferevent *bev, YAEvent *event)
{
#ifndef YAYA_TESTS
    if (!event || !event->param)
    {
        YA_LOG_ERROR("Invalid mouse move event or parameters");
        return NULL;
    }

    const YACommonEventRequest *request = (YACommonEventRequest *)event->param;
    if (event->param_len != sizeof(YACommonEventRequest))
    {
        YA_LOG_ERROR("Invalid mouse move parameter size");
        return NULL;
    }

    ya_client_t *client = ya_client_find_by_uid(&svr_context.client_manager, event->header.uid);
    if (!client)
    {
        YA_LOG_WARN("Mouse move: client not found for uid=%u, Ignore.", event->header.uid);
        return NULL;
    }


    // 将客户端放大后的相对位移缩小还原为像素，再交给服务端滤波模块
    const double kRecvScale = 100.0; // 与前端 MouseController.kSendScale 保持一致
    const int rx = (int)request->lparam;
    const int ry = (int)request->rparam;
    const double fdx = ((double)rx) / kRecvScale;
    const double fdy = ((double)ry) / kRecvScale;
    const int dx_px = (int)(fdx >= 0.0 ? fdx + 0.5 : fdx - 0.5); // 四舍五入为像素
    const int dy_px = (int)(fdy >= 0.0 ? fdy + 0.5 : fdy - 0.5);

    int out_dx = dx_px;
    int out_dy = dy_px;
    if (ya_is_wayland_session())
    {
        if (!ya_mouse_throttle_collect(dx_px, dy_px, false, &out_dx, &out_dy))
        {
            return NULL;
        }
    }

    ya_mouse_step_t steps[128];
    size_t out_n = 0;

    if (client->mouse_filter)
    {
        // 交给滤波模块生成若干微步
        ya_mouse_filter_process(client->mouse_filter, out_dx, out_dy, steps, (sizeof(steps) / sizeof(steps[0])), &out_n);
    }
    else 
    {
        steps[0].dx = out_dx;
        steps[0].dy = out_dy;
        out_n = 1;
    }

    // 按顺序执行微步，保持轨迹连续
    for (size_t i = 0; i < out_n; ++i)
    {
        int dx = steps[i].dx;
        int dy = steps[i].dy;
        if (dx == 0 && dy == 0)
            continue;
        enum YAError e = Success;
        e = move_mouse(dx, dy, Rel);
        if (e != Success)
        {
            YA_LOG_ERROR("Failed to move mouse (step %zu/%zu): %d", i + 1, out_n, e);
        }
    }
#endif
    return NULL;
}

YAEvent *handle_mouse_click(struct bufferevent *bev, YAEvent *event)
{
#ifndef YAYA_TESTS
    if (!event || !event->param)
    {
        YA_LOG_ERROR("Invalid mouse click event or parameters");
        return NULL;
    }

    const YACommonEventRequest *request = (YACommonEventRequest *)event->param;
    if (event->param_len != sizeof(YACommonEventRequest))
    {
        YA_LOG_ERROR("Invalid mouse click parameter size");
        return NULL;
    }

    // 按照更新后的协议：
    // lparam 按钮: 0=Left,1=Right,2=Middle,3=Back,4=Forward,5..8 为滚轮方向（暂不在点击里支持）
    // rparam 操作: 0=Press,1=Release,2=Click,3=DoubleClick
    enum CButton btn;
    switch (request->lparam)
    {
    case 0:
        btn = Left;
        break;
    case 1:
        btn = Right;
        break;
    case 2:
        btn = Middle;
        break;
    default:
        YA_LOG_ERROR("Invalid mouse button (YA): %d", request->lparam);
        return NULL;
    }

    enum CDirection dir;
    switch (request->rparam)
    {
    case 0:
        dir = Press;
        break;
    case 1:
        dir = Release;
        break;
    case 2:
        dir = Click;
        break;
    case 3: // DoubleClick: 暂时退化为 Click，必要时可扩展 rs.h 支持
        dir = Click;
        break;
    default:
        YA_LOG_ERROR("Invalid click op (YA): %d", request->rparam);
        return NULL;
    }

    YA_LOG_TRACE("Mouse click mapped {btn=%d, dir=%d} from {lparam=%d, rparam=%d}", (int)btn, (int)dir, request->lparam,
                 request->rparam);
    enum YAError err = Success;
    if (request->rparam == 3)
    {
        // DoubleClick: 连续两次 Click
        err = mouse_button(btn, Click);
        if (err == Success)
        {
            err = mouse_button(btn, Click);
        }
    }
    else
    {
        err = mouse_button(btn, dir);
    }
#endif
    return NULL;
}

YAEvent *handle_mouse_scroll(struct bufferevent *bev, YAEvent *event)
{
#ifndef YAYA_TESTS
    if (!event || !event->param)
    {
        YA_LOG_ERROR("Invalid mouse scroll event or parameters");
        return NULL;
    }

    const YACommonEventRequest *request = (YACommonEventRequest *)event->param;
    if (event->param_len != sizeof(YACommonEventRequest))
    {
        YA_LOG_ERROR("Invalid mouse scroll parameter size");
        return NULL;
    }

    // 协议：lparam 为距离（步数，严格正数），rparam 为方向编码：0 上, 1 下, 2 左, 3 右
    int amount = request->lparam;
    if (amount <= 0)
    {
        YA_LOG_ERROR("Mouse wheel invalid amount: %d (must be > 0)", amount);
        return NULL;
    }

    enum CButton wheel_btn;
    switch (request->rparam)
    {
    case 0:
        wheel_btn = ScrollUp;
        break;
    case 1:
        wheel_btn = ScrollDown;
        break;
    case 2:
        wheel_btn = ScrollLeft;
        break;
    case 3:
        wheel_btn = ScrollRight;
        break;
    default:
        YA_LOG_ERROR("Invalid wheel direction: %d", request->rparam);
        return NULL;
    }

    int steps = amount;
    if (steps > 1000)
        steps = 1000;

    const char *dir_str = (wheel_btn == ScrollUp)     ? "up"
                          : (wheel_btn == ScrollDown) ? "down"
                          : (wheel_btn == ScrollLeft) ? "left"
                                                      : "right";
    YA_LOG_TRACE("Mouse wheel amount=%d, direction=%s", steps, dir_str);
    for (int i = 0; i < steps; ++i)
    {
        enum YAError err = Success;
        err = mouse_button(wheel_btn, Click);
        if (err != Success)
        {
            YA_LOG_ERROR("Failed to scroll mouse wheel at step %d/%d: %d", i + 1, steps, err);
        }
    }
#endif
    return NULL;
}

YAEvent *handle_keyboard(struct bufferevent *bev, YAEvent *event)
{
#ifndef YAYA_TESTS
    if (!event || !event->param)
    {
        YA_LOG_ERROR("Invalid key click event or parameters");
        return NULL;
    }

    const YAKeyboardEventRequest *req = (const YAKeyboardEventRequest *)event->param;
    if (event->param_len != sizeof(YAKeyboardEventRequest))
    {
        YA_LOG_ERROR("Invalid keyboard parameter size: %zu", (size_t)event->param_len);
        return NULL;
    }

    // 操作类型
    enum CDirection dir;
    switch (req->op)
    {
    case 0:
        dir = Press;
        break;
    case 1:
        dir = Release;
        break;
    case 2:
        dir = Click;
        break;
    default:
        YA_LOG_ERROR("Invalid key op (YA): %d", req->op);
        return NULL;
    }

    enum CKey base_key;
    bool is_ascii = (req->code >= 0x20 && req->code <= 0x7E);
    if (!is_ascii)
    {
        if (!ckey_from_protocol(req->code, &base_key))
        {
            YA_LOG_ERROR("Unknown YA key code: %d", req->code);
            return NULL;
        }
    }

    // 修饰位
    bool mod_shift = (req->mods & CHORD_MOD_SHIFT) != 0;
    bool mod_ctrl = (req->mods & CHORD_MOD_CTRL) != 0;
    bool mod_alt = (req->mods & CHORD_MOD_ALT) != 0;
    bool mod_meta = (req->mods & CHORD_MOD_META) != 0;
    bool mod_fn = (req->mods & CHORD_MOD_FN) != 0;

    // 执行序列（加锁，保证在多客户端下按键注入的原子性与顺序）
    ya_key_lock();
    if (dir == Click)
    {
        bool pressed_shift = false, pressed_ctrl = false, pressed_alt = false, pressed_meta = false;
        if (mod_shift)
        {
            (void)key_action(Shift, Press);
            pressed_shift = true;
            ya_key_sleep_ms(YA_KEY_DELAY_MS);
        }
        if (mod_ctrl)
        {
            (void)key_action(Control, Press);
            pressed_ctrl = true;
            ya_key_sleep_ms(YA_KEY_DELAY_MS);
        }
        if (mod_alt)
        {
            (void)key_action(Alt, Press);
            pressed_alt = true;
            ya_key_sleep_ms(YA_KEY_DELAY_MS);
        }
        if (mod_meta)
        {
            (void)key_action(Meta, Press);
            pressed_meta = true;
            ya_key_sleep_ms(YA_KEY_DELAY_MS);
        }

        // 与主键之间留出有效间隔
        ya_key_sleep_ms(YA_KEY_DELAY_MS);
        if (is_ascii)
        {
            (void)ya_inject_ascii((int32_t)req->code, Click);
            ya_key_sleep_ms(YA_KEY_DELAY_MS);
        }
        else
        {
            // 显式地分解 Click 为 Press/Release，可控间隔
            (void)key_action(base_key, Press);
            ya_key_sleep_ms(YA_KEY_DELAY_MS);
            (void)key_action(base_key, Release);
            ya_key_sleep_ms(YA_KEY_DELAY_MS);
        }

        if (pressed_meta)
        {
            (void)key_action(Meta, Release);
            ya_key_sleep_ms(YA_KEY_DELAY_MS);
        }
        if (pressed_alt)
        {
            (void)key_action(Alt, Release);
            ya_key_sleep_ms(YA_KEY_DELAY_MS);
        }
        if (pressed_ctrl)
        {
            (void)key_action(Control, Release);
            ya_key_sleep_ms(YA_KEY_DELAY_MS);
        }
        if (pressed_shift)
        {
            (void)key_action(Shift, Release);
            ya_key_sleep_ms(YA_KEY_DELAY_MS);
        }
    }
    else if (dir == Press)
    {
        if (mod_shift)
        {
            (void)key_action(Shift, Press);
            ya_key_sleep_ms(YA_KEY_DELAY_MS);
        }
        if (mod_ctrl)
        {
            (void)key_action(Control, Press);
            ya_key_sleep_ms(YA_KEY_DELAY_MS);
        }
        if (mod_alt)
        {
            (void)key_action(Alt, Press);
            ya_key_sleep_ms(YA_KEY_DELAY_MS);
        }
        if (mod_meta)
        {
            (void)key_action(Meta, Press);
            ya_key_sleep_ms(YA_KEY_DELAY_MS);
        }

        ya_key_sleep_ms(YA_KEY_DELAY_MS);
        if (is_ascii)
        {
            (void)ya_inject_ascii((int32_t)req->code, Press);
            ya_key_sleep_ms(YA_KEY_DELAY_MS);
        }
        else
        {
            (void)key_action(base_key, Press);
            ya_key_sleep_ms(YA_KEY_DELAY_MS);
        }
    }
    else
    {
        // Release 顺序：先主键，后修饰键
        if (is_ascii)
        {
            (void)ya_inject_ascii((int32_t)req->code, Release);
            ya_key_sleep_ms(YA_KEY_DELAY_MS);
        }
        else
        {
            (void)key_action(base_key, Release);
            ya_key_sleep_ms(YA_KEY_DELAY_MS);
        }
        if (mod_meta)
        {
            (void)key_action(Meta, Release);
            ya_key_sleep_ms(YA_KEY_DELAY_MS);
        }
        if (mod_alt)
        {
            (void)key_action(Alt, Release);
            ya_key_sleep_ms(YA_KEY_DELAY_MS);
        }
        if (mod_ctrl)
        {
            (void)key_action(Control, Release);
            ya_key_sleep_ms(YA_KEY_DELAY_MS);
        }
        if (!is_ascii && mod_shift)
        {
            (void)key_action(Shift, Release);
            ya_key_sleep_ms(YA_KEY_DELAY_MS);
        }
    }
    ya_key_unlock();
#endif
    return NULL;
}

YAEvent *handle_input_text(struct bufferevent *bev, YAEvent *event)
{
#ifndef YAYA_TESTS
    const YATextInputEventRequest *request = (YATextInputEventRequest *)event->param;

    // 读取配置开关：[input].clipboard_fallback
    bool fallback_enabled = false;
    {
        const char* cf = ya_config_get(&config, "input", "clipboard_fallback");
        if (cf && (strcmp(cf, "1") == 0 || strcmp(cf, "true") == 0 || strcmp(cf, "on") == 0 || strcmp(cf, "yes") == 0))
        {
            fallback_enabled = true;
        }
    }

    // 首先尝试直接插入文本
    enum YAError err = enter_text(request->text);
    if (err != Success && fallback_enabled)
    {
        YA_LOG_DEBUG("enter_text failed (%d), trying clipboard fallback...", (int)err);

        // 备份现有剪贴板（允许失败）
        const char *backup = clipboard_get();

        // 设置剪贴板为要输入的文本
        enum YAError cerr = clipboard_set(request->text);
        if (cerr != Success)
        {
            YA_LOG_WARN("clipboard_set failed in fallback: %d", (int)cerr);
        }
        else
        {
            // 发送 Shift+Insert 粘贴序列
            ya_key_lock();
            (void)key_action(Shift, Press);
            ya_key_sleep_ms(YA_KEY_DELAY_MS);
            (void)key_action(Insert, Click);
            ya_key_sleep_ms(YA_KEY_DELAY_MS);
            (void)key_action(Shift, Release);
            ya_key_sleep_ms(YA_KEY_DELAY_MS);
            ya_key_unlock();
        }

        // 恢复剪贴板（尽力而为）
        if (backup)
        {
            enum YAError rerr = clipboard_set(backup);
            if (rerr != Success)
            {
                YA_LOG_TRACE("restore clipboard failed: %d", (int)rerr);
            }
            free_string((char *)backup);
        }
    }
#endif
    return NULL;
}

YAEvent *handle_input_get(struct bufferevent *bev, YAEvent *event)
{
#ifndef YAYA_TESTS
    YAEvent *response_event = assign_response(event, sizeof(YAInputGetEventResponse));
    YAInputGetEventResponse *response = response_event->param;
    enum YAError err;

#ifdef __APPLE__
    enum CKey modifier = Meta;
#else
    enum CKey modifier = Control;
#endif

    // 全选并复制: Modifier + A, Modifier + C（静默执行）
    err = key_action(modifier, Press);
    if (err != Success)
    {
        goto cleanup;
    }

    err = key_action_with_code('a', Click);
    if (err != Success)
    {
        key_action(modifier, Release);
        goto cleanup;
    }

    err = key_action_with_code('c', Click);
    if (err != Success)
    {
        key_action(modifier, Release);
        goto cleanup;
    }

    err = key_action(modifier, Release);
    if (err != Success)
    {
        goto cleanup;
    }

    // 取消选择
    err = mouse_button(Left, Click);
    if (err != Success)
    {
        goto cleanup;
    }

    // 获取剪贴板内容
    const char *clipboard_text = clipboard_get();
    if (clipboard_text == NULL)
    {
        goto cleanup;
    }

    // 复制文本到响应结构
    response->text = strdup(clipboard_text);
    if (response->text == NULL)
    {
        free_string((char *)clipboard_text);
        goto cleanup;
    }

    free_string((char *)clipboard_text);
    return response_event;

cleanup:
    ya_free_event(response_event);
    return NULL;
#else
    return NULL;
#endif
}

YAEvent *handle_poweroff(struct bufferevent *bev, YAEvent *event)
{
    YA_LOG_TRACE("Power (shutdown) request received");

    YAEvent *response = assign_response(event, 0);
    if (!response)
    {
        YA_LOG_ERROR("Failed to allocate response event");
        return NULL;
    }

    int ret = ya_run_power_script("poweroff");
    if (ret != 0)
    {
#if defined(_WIN32) || defined(_WIN64)
        ret = system("shutdown /s /t 0");
#elif defined(__APPLE__)
        ret = system("shutdown -h now");
#elif defined(__linux__)
        ret = system("shutdown -h now");
#else
        ret = -1;
#endif
    }

    if (ret != 0)
    {
        YA_LOG_WARN("Shutdown command returned code %d", ret);
    }

    return response;
}

YAEvent *handle_restart(struct bufferevent *bev, YAEvent *event)
{
    YA_LOG_TRACE("Restart request received");

    YAEvent *response = assign_response(event, 0);
    if (!response)
    {
        YA_LOG_ERROR("Failed to allocate response event");
        return NULL;
    }

    int ret = ya_run_power_script("restart");
    if (ret != 0)
    {
#if defined(_WIN32) || defined(_WIN64)
        ret = system("shutdown /r /t 0");
#elif defined(__APPLE__)
        ret = system("shutdown -r now");
#elif defined(__linux__)
        ret = system("reboot");
#else
        ret = -1;
#endif
    }

    if (ret != 0)
    {
        YA_LOG_WARN("Restart command returned code %d", ret);
    }

    return response;
}

YAEvent *handle_sleep(struct bufferevent *bev, YAEvent *event)
{
    YA_LOG_TRACE("Sleep request received");

    YAEvent *response = assign_response(event, 0);
    if (!response)
    {
        YA_LOG_ERROR("Failed to allocate response event");
        return NULL;
    }

    int ret = ya_run_power_script("sleep");
    if (ret != 0)
    {
#if defined(_WIN32) || defined(_WIN64)
        ret = system("rundll32.exe powrprof.dll,SetSuspendState 0,1,0");
#elif defined(__APPLE__)
        ret = system("pmset sleepnow");
#elif defined(__linux__)
        ret = system("systemctl suspend");
#else
        ret = -1;
#endif
    }

    if (ret != 0)
    {
        YA_LOG_WARN("Sleep command returned code %d", ret);
    }

    return response;
}


// CONTROL 统一入口（跨平台可见）：根据 lparam 选择具体电源操作
YAEvent *handle_control(struct bufferevent *bev, YAEvent *event)
{
    (void)bev;
    if (!event || !event->param)
    {
        YA_LOG_WARN("CONTROL event missing parameters");
        return NULL;
    }

    YACommonEventRequest *req = (YACommonEventRequest *)event->param;
    int op = req ? req->lparam : 0;
    switch (op)
    {
    case CONTROL_OP_SHUTDOWN:
        return handle_poweroff(bev, event);
    case CONTROL_OP_RESTART:
        return handle_restart(bev, event);
    case CONTROL_OP_SLEEP:
        return handle_sleep(bev, event);
    default:
        YA_LOG_WARN("Unknown CONTROL op: %d", op);
        return assign_response(event, 0);
    }
}

YAEvent *handle_discover(struct bufferevent *bev, YAEvent *event)
{
    YADiscoverEventRequest *request = event->param;

    if (request->magic != DISCOVERY_MAGIC)
    {
        return NULL;
    }

    YAEvent *response = assign_response(event, sizeof(YADiscoverEventResponse));
    YADiscoverEventResponse *discoverResponse = response->param;
    discoverResponse->magic = DISCOVERY_MAGIC;
    discoverResponse->port = ntohs(svr_context.session_sock_addr.sin_port);

    return response;
}

// 事件处理函数映射表
static const struct
{
    YAEventType type;
    ya_event_handler_t handler;
} event_handlers[] = {
    {MOUSE_MOVE, handle_mouse_move},
    {MOUSE_CLICK, handle_mouse_click},
    {MOUSE_WHEEL, handle_mouse_scroll},
    {KEYBOARD, handle_keyboard}, 
    {TEXT_INPUT, handle_input_text},
    {TEXT_GET, handle_input_get},
    {DISCOVER, handle_discover},
    {AUTHORIZE, handle_authorize},
    {HEARTBEAT, handle_heartbeat},
    {SESSION_OPTION, handle_session_option},
    {CONTROL, handle_control}
};

// 查找事件处理函数
static ya_event_handler_t find_event_handler(YAEventType type)
{
    for (size_t i = 0; i < sizeof(event_handlers) / sizeof(event_handlers[0]); i++)
    {
        if (event_handlers[i].type == type)
        {
            return event_handlers[i].handler;
        }
    }
    return NULL;
}

// 检查事件是否需要命令序号验证
static bool need_command_index_check(YAEventType type)
{
    // DISCOVER和AUTHORIZE是基础事件，不需要命令序号验证
    return type != DISCOVER && type != AUTHORIZE && type != HEARTBEAT;
}

// 验证命令序号
static bool validate_command_index(ya_client_t *client, YAEvent *event)
{
    if (!client || event->header.direction != REQUEST || !need_command_index_check(event->header.type))
    {
        return true;
    }

    // 如果命令序号小于等于当前序号，说明是旧命令，丢弃
    if (event->header.index <= client->command_index)
    {
        YA_LOG_TRACE("Discarding old command with index %d (current index: %d)", event->header.index,
                     client->command_index);
        return false;
    }

    // 更新命令序号
    client->command_index = event->header.index;
    return true;
}

YAEvent *process_server_event(struct bufferevent *bev, YAEvent *event, ya_client_t *client)
{
    if (!event)
    {
        return NULL;
    }

    ya_event_handler_t handler = find_event_handler(event->header.type);
    if (!handler)
    {
        YA_LOG_ERROR("Unknown event type: %d", event->header.type);
        return NULL;
    }

    // 对于非AUTHORIZE和DISCOVER事件，检查命令序号
    if (client && event->header.direction == REQUEST && need_command_index_check(event->header.type))
    {
        if (event->header.index <= client->command_index)
        {
            YA_LOG_TRACE("Discarding old command with index %d (current index: %d)", event->header.index,
                         client->command_index);
            return NULL;
        }
        client->command_index = event->header.index;
    }

    if (event->header.type != HEARTBEAT) {
        YA_LOG_TRACE("Processing event type: %d, index: %d", event->header.type, event->header.index);
    }
    return handler(bev, event);
}
