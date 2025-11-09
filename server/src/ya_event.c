#include "ya_event.h"
#include "mpack/mpack.h"
#include "ya_utils.h"

#include <event2/buffer.h>
#include <stddef.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>

typedef int (*serializer_fn)(const void *param, size_t param_len, mpack_writer_t *writer);
typedef int (*parser_fn)(const char *data, size_t len, void **out_param, size_t *out_len);

// Forward declarations for event-specific (de)serializers
static int serialize_common_request(const void *param, size_t param_len, mpack_writer_t *writer);
static int parse_common_request(const char *data, size_t len, void **out_param, size_t *out_len);

static int serialize_text_input_request(const void *param, size_t param_len, mpack_writer_t *writer);
static int parse_text_input_request(const char *data, size_t len, void **out_param, size_t *out_len);

static int serialize_text_get_response(const void *param, size_t param_len, mpack_writer_t *writer);
static int parse_text_get_response(const char *data, size_t len, void **out_param, size_t *out_len);

static int serialize_authorize_request(const void *param, size_t param_len, mpack_writer_t *writer);
static int parse_authorize_request(const char *data, size_t len, void **out_param, size_t *out_len);
static int serialize_authorize_response(const void *param, size_t param_len, mpack_writer_t *writer);
static int parse_session_option_request(const char *data, size_t len, void **out_param, size_t *out_len);

static int parse_discover_request(const char *data, size_t len, void **out_param, size_t *out_len);
static int serialize_discover_response(const void *param, size_t param_len, mpack_writer_t *writer);

// Keyboard (code, op, mods)
static int serialize_keyboard_request(const void *param, size_t param_len, mpack_writer_t *writer);
static int parse_keyboard_request(const char *data, size_t len, void **out_param, size_t *out_len);

// Dispatcher for serializers based on event type and direction
static serializer_fn get_serializer(YAEventType type, YAEventDirection dir)
{
    switch (type)
    {
    case MOUSE_MOVE:
    case MOUSE_CLICK:
    case MOUSE_WHEEL:
    case CONTROL:
        return serialize_common_request;
    case KEYBOARD:
        return serialize_keyboard_request;
    case TEXT_INPUT:
        return serialize_text_input_request;
    case TEXT_GET:
        return serialize_text_get_response;
    case AUTHORIZE:
        return serialize_authorize_response;
    case DISCOVER:
        return serialize_discover_response;
    case HEARTBEAT:
        return NULL; // no payload
    default:
        return NULL;
    }
}

// Dispatcher for parsers based on event type and direction
static parser_fn get_parser(YAEventType type, YAEventDirection dir)
{
    switch (type)
    {
    case MOUSE_MOVE:
    case MOUSE_CLICK:
    case MOUSE_WHEEL:
    case CONTROL:
        return parse_common_request;
    case KEYBOARD:
        return parse_keyboard_request;
    case TEXT_INPUT:
        return parse_text_input_request;
    case TEXT_GET:
        return parse_text_get_response;
    case AUTHORIZE:
        return parse_authorize_request;
    case SESSION_OPTION:
        return parse_session_option_request;
    case DISCOVER:
        return parse_discover_request;
    case HEARTBEAT:
        return NULL;
    default:
        return NULL;
    }
}

int ya_parse_event_header(struct evbuffer *buf, YAEventHeader *out_header, uint32_t length)
{
    if (evbuffer_get_length(buf) < length)
    {
        return -1;
    }

    unsigned char *data = evbuffer_pullup(buf, length);
    if (!data)
    {
        return -1;
    }

    mpack_reader_t reader;
    mpack_reader_init_data(&reader, (const char *)data, length);
    uint32_t count = mpack_expect_array(&reader);
    if (count != 4)
    {
        mpack_reader_flag_error(&reader, mpack_error_type);
    }

    uint32_t uid = mpack_expect_i32(&reader);
    uint8_t type = (uint8_t)mpack_expect_i32(&reader);
    uint8_t direction = (uint8_t)mpack_expect_i32(&reader);
    uint32_t index = mpack_expect_i32(&reader);
    mpack_done_array(&reader);

    if (mpack_reader_error(&reader) != mpack_ok)
    {
        return -1;
    }
    mpack_reader_destroy(&reader);

    // if (buf_len < header_bytes + content_len)
    //     return -1;

    out_header->uid = uid;
    out_header->type = (YAEventType)type;
    out_header->direction = (YAEventDirection)direction;
    out_header->index = index;

    return 0;
}

int ya_parse_event(struct evbuffer *buf, YAEvent *event, YAPackageSize *size)
{
    YAEventHeader header;
    if (ya_parse_event_header(buf, &header, size->headerSize) < 0)
    {
        return -1;
    }

    evbuffer_drain(buf, size->headerSize);

    int content_len = size->bodySize;

    if (content_len == 0)
    {
        // no body
        event->header = header;
        return 0;
    }

    unsigned char *payload = evbuffer_pullup(buf, content_len);
    if (!payload)
    {
        return -1;
    }

    parser_fn parser = get_parser(header.type, header.direction);
    void *param = NULL;
    size_t param_len = 0;
    if (parser && parser((const char *)payload, content_len, &param, &param_len) < 0)
    {
        return -1;
    }

    evbuffer_drain(buf, content_len);

    event->header = header;
    event->param = param;
    event->param_len = param_len;

    return 0;
}

int ya_serialize_event(YAEvent *event, uint8_t **out)
{
    // Serialize payload
    mpack_writer_t pw = {0};
    char *body_buf = NULL;
    size_t body_size = 0;
    mpack_writer_init_growable(&pw, &body_buf, &body_size);
    serializer_fn ser = get_serializer(event->header.type, event->header.direction);
    if (ser && ser(event->param, event->param_len, &pw) < 0)
    {
        mpack_writer_destroy(&pw);
        return -1;
    }
    mpack_writer_destroy(&pw);

    // Serialize header
    mpack_writer_t hw = {0};
    char *hdr_buf = NULL;
    size_t hdr_size = 0;
    mpack_writer_init_growable(&hw, &hdr_buf, &hdr_size);
    mpack_start_array(&hw, 4);
    mpack_write_u32(&hw, event->header.uid);
    mpack_write_u32(&hw, (uint32_t)event->header.type);
    mpack_write_u32(&hw, (uint32_t)event->header.direction);
    mpack_write_u32(&hw, event->header.index);
    mpack_finish_array(&hw);
    mpack_writer_destroy(&hw);

    // Combine
    size_t total = hdr_size + body_size + 4;
    int total_with_size = total + 4;
    *out = malloc(total_with_size);
    memset(*out, 0, total_with_size);

    uint32_t _total = (uint32_t)htonl(total);
    uint32_t _hdr_size = (uint32_t)htonl(hdr_size);

    int start = 0;
    memcpy(*out + start, &_total, 4);
    start += 4;

    memcpy(*out + start, &_hdr_size, 4);
    start += 4;

    memcpy(*out + start, hdr_buf, hdr_size);
    start += hdr_size;

    memcpy(*out + start, body_buf, body_size);

    // Cleanup
    free(body_buf);
    free(hdr_buf);

    return total_with_size;
}

void ya_free_event_param(YAEvent *event)
{
    if (!event || !event->param)
    {
        return;
    }

    switch (event->header.type)
    {
    case TEXT_INPUT:
        free(((YATextInputEventRequest *)event->param)->text);
        break;
    case TEXT_GET:
        free(((YAInputGetEventResponse *)event->param)->text);
        break;
    case AUTHORIZE:
        if (event->header.direction == RESPONSE)
        {
            YAAuthorizeEventResponse *resp = (YAAuthorizeEventResponse *)event->param;
            if (resp->macs)
            {
                free(resp->macs);
            }
        }
        free(event->param);
        break;
    case DISCOVER:
        free(event->param);
        break;
    case SESSION_OPTION:
        free(event->param);
        break;
    default:
        free(event->param);
    }
    event->param = NULL;
}

void ya_free_event(YAEvent *event)
{
    if (!event)
    {
        return;
    }
    ya_free_event_param(event);
    free(event);
}

// --- Serializers/Parsers implementations ---

// Common mouse/keyboard events
static int serialize_common_request(const void *param, size_t unused, mpack_writer_t *writer)
{
    const YACommonEventRequest *req = param;
    mpack_start_array(writer, 2);
    mpack_write_i32(writer, req->lparam);
    mpack_write_i32(writer, req->rparam);
    mpack_finish_array(writer);
    return 0;
}
static int parse_common_request(const char *data, size_t len, void **out_param, size_t *out_len)
{
    mpack_reader_t r;
    mpack_reader_init_data(&r, data, len);
    mpack_expect_array(&r);
    YACommonEventRequest *req = malloc(sizeof(*req));
    req->lparam = mpack_expect_i32(&r);
    req->rparam = mpack_expect_i32(&r);
    mpack_done_array(&r);
    mpack_reader_destroy(&r);
    *out_param = req;
    *out_len = sizeof(*req);
    return 0;
}

// Keyboard (code, op, mods)
static int serialize_keyboard_request(const void *param, size_t unused, mpack_writer_t *writer)
{
    const YAKeyboardEventRequest *req = param;
    mpack_start_array(writer, 3);
    mpack_write_i32(writer, req->code);
    mpack_write_i32(writer, req->op);
    mpack_write_u32(writer, req->mods);
    mpack_finish_array(writer);
    return 0;
}

static int parse_keyboard_request(const char *data, size_t len, void **out_param, size_t *out_len)
{
    mpack_reader_t r;
    mpack_reader_init_data(&r, data, len);
    mpack_expect_array(&r);
    YAKeyboardEventRequest *req = malloc(sizeof(*req));
    req->code = mpack_expect_i32(&r);
    req->op = mpack_expect_i32(&r);
    req->mods = mpack_expect_u32(&r);
    mpack_done_array(&r);
    mpack_reader_destroy(&r);
    *out_param = req;
    *out_len = sizeof(*req);
    return 0;
}

// Text input request
static int serialize_text_input_request(const void *param, size_t unused, mpack_writer_t *writer)
{
    const YATextInputEventRequest *req = param;
    mpack_write_str(writer, req->text, strlen(req->text));
    return 0;
}
static int parse_text_input_request(const char *data, size_t len, void **out_param, size_t *out_len)
{
    mpack_reader_t r;
    mpack_reader_init_data(&r, data, len);
    uint32_t str_len = mpack_expect_str(&r);
    char *text = malloc(str_len + 1);
    mpack_read_bytes(&r, text, str_len);
    text[str_len] = '\0';
    mpack_done_str(&r);
    mpack_reader_destroy(&r);
    YATextInputEventRequest *req = malloc(sizeof(*req));
    req->text = text;
    *out_param = req;
    *out_len = sizeof(*req);
    return 0;
}

// Text get response
static int serialize_text_get_response(const void *param, size_t unused, mpack_writer_t *writer)
{
    const YAInputGetEventResponse *resp = param;
    mpack_write_str(writer, resp->text, strlen(resp->text));
    return 0;
}

static int parse_text_get_response(const char *data, size_t len, void **out_param, size_t *out_len)
{
    mpack_reader_t r;
    mpack_reader_init_data(&r, data, len);
    uint32_t str_len = mpack_expect_str(&r);
    char *text = malloc(str_len + 1);
    mpack_read_bytes(&r, text, str_len);
    text[str_len] = '\0';
    mpack_done_str(&r);
    mpack_reader_destroy(&r);
    YAInputGetEventResponse *resp = malloc(sizeof(*resp));
    resp->text = text;
    *out_param = resp;
    *out_len = sizeof(*resp);
    return 0;
}

// Authorize request/response
static int serialize_authorize_request(const void *param, size_t unused, mpack_writer_t *writer)
{
    const YAAuthorizeEventRequest *req = param;
    mpack_start_array(writer, 2);
    mpack_write_u32(writer, (uint32_t)req->type);
    mpack_write_u32(writer, req->version);
    mpack_finish_array(writer);
    return 0;
}
static int parse_authorize_request(const char *data, size_t len, void **out_param, size_t *out_len)
{
    mpack_reader_t r;
    mpack_reader_init_data(&r, data, len);
    mpack_expect_array(&r);
    YAAuthorizeEventRequest *req = malloc(sizeof(*req));
    memset(req, 0, sizeof(*req));
    req->type = (YAClientType)mpack_expect_u32(&r);
    req->version = mpack_expect_u32(&r);
    mpack_done_array(&r);
    mpack_reader_destroy(&r);
    *out_param = req;
    *out_len = sizeof(*req);
    return 0;
}
static int parse_session_option_request(const char *data, size_t len, void **out_param, size_t *out_len)
{
    mpack_reader_t r;
    mpack_reader_init_data(&r, data, len);
    mpack_expect_array(&r);
    YASessionOptionEventRequest *req = malloc(sizeof(*req));
    if (!req)
    {
        mpack_reader_destroy(&r);
        return -1;
    }
    memset(req, 0, sizeof(*req));
    // 数组: [pointerScale]
    if (mpack_reader_error(&r) == mpack_ok) {
        req->pointer_scale = mpack_expect_float(&r);
    }
    mpack_done_array(&r);
    mpack_reader_destroy(&r);
    *out_param = req;
    *out_len = sizeof(*req);
    return 0;
}
static int serialize_authorize_response(const void *param, size_t unused, mpack_writer_t *writer)
{
    const YAAuthorizeEventResponse *resp = param;
    
    // 根据客户端版本决定响应格式
    // client_version 字段存储的是客户端版本
    if (resp->client_version >= 2)
    {
        // Protocol v2: 8 fields [success, os_type, uid, session_port, command_port, macs, display_scale, protocol_version]
        mpack_start_array(writer, 8);
        mpack_write_bool(writer, resp->success);
        mpack_write_u32(writer, resp->os_type);
        mpack_write_u32(writer, resp->uid);
        mpack_write_u32(writer, (uint32_t)resp->session_port);
        mpack_write_u32(writer, (uint32_t)resp->command_port);
        const char *macs = resp->macs ? resp->macs : "";
        mpack_write_str(writer, macs, strlen(macs));
        mpack_write_float(writer, resp->display_scale);
        mpack_write_u32(writer, YA_PROTOCOL_VERSION); // 发送服务端协议版本
        mpack_finish_array(writer);
    }
    else
    {
        // Protocol v1: 7 fields [success, os_type, uid, session_port, command_port, macs, display_scale]
        // 兼容旧客户端
        mpack_start_array(writer, 7);
        mpack_write_bool(writer, resp->success);
        mpack_write_u32(writer, resp->os_type);
        mpack_write_u32(writer, resp->uid);
        mpack_write_u32(writer, (uint32_t)resp->session_port);
        mpack_write_u32(writer, (uint32_t)resp->command_port);
        const char *macs = resp->macs ? resp->macs : "";
        mpack_write_str(writer, macs, strlen(macs));
        mpack_write_float(writer, resp->display_scale);
        mpack_finish_array(writer);
    }
    
    return 0;
}

static int parse_discover_request(const char *data, size_t len, void **out_param, size_t *out_len)
{
    mpack_reader_t r;
    mpack_reader_init_data(&r, data, len);
    YADiscoverEventRequest *req = malloc(sizeof(*req));
    req->magic = mpack_expect_u32(&r);
    mpack_reader_destroy(&r);
    *out_param = req;
    *out_len = sizeof(*req);
    return 0;
}
static int serialize_discover_response(const void *param, size_t unused, mpack_writer_t *writer)
{
    const YADiscoverEventResponse *resp = param;
    mpack_start_array(writer, 2);
    mpack_write_u32(writer, DISCOVERY_MAGIC);
    mpack_write_u32(writer, resp->port);
    mpack_finish_array(writer);
    return 0;
}

int ya_get_package_size(struct evbuffer *buffer, YAPackageSize *info)
{
    uint32_t totalSize = to_uint32(buffer, 0);
    uint32_t headerSize = to_uint32(buffer, 4);

    info->totalSize = totalSize;
    info->headerSize = headerSize;
    info->bodySize = totalSize - headerSize - sizeof(uint32_t);

    return 0;
}
