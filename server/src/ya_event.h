#pragma once

#include <event2/buffer.h>
#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

/**
 *
 * Protocol defines
 *
 * Header Part
 *
 * | Content-Length (32 bit) | UID (32 bit) | Event-Type (8 bit) | Direction (8
 * bit) | Index (32 bit) | Payload |
 *
 *
 * UDP Events
 *
 * */

#define YA_HEADER_CONTENT_LENGTH_SIZE 32 / 8
#define YA_HEADER_UID_SIZE 32 / 8
#define YA_HEADER_EVENT_TYPE_SIZE 8 / 8
#define YA_HEADER_DIRECTION_SIZE 8 / 8
#define YA_HEADER_INDEX_SIZE 32 / 8
#define YA_HEADER_SIZE                                                                                                 \
    YA_HEADER_CONTENT_LENGTH_SIZE + YA_HEADER_UID_SIZE + YA_HEADER_EVENT_TYPE_SIZE + YA_HEADER_DIRECTION_SIZE +        \
        YA_HEADER_INDEX_SIZE

#define DISCOVERY_MAGIC 0x4B424D43

// Protocol version for client-server compatibility check
#define YA_PROTOCOL_VERSION 2

typedef enum
{
    MOUSE_MOVE = 0x1,
    MOUSE_CLICK = 0x2,
    MOUSE_WHEEL = 0x3,
    KEYBOARD = 0x4,

    TEXT_INPUT = 0x5,
    TEXT_GET = 0x6,

    DISCOVER = 0x7,
    MOUSE_STOP = 0x8,
    CONTROL = 0x9,
    AUTHORIZE = 0xA,
    HEARTBEAT = 0xB,
    SESSION_OPTION = 0xC,
} YAEventType;

typedef enum
{
    REQUEST = 0x1,
    RESPONSE = 0x2,
} YAEventDirection;

typedef struct
{
    YAEventType type;

    YAEventDirection direction;

    uint32_t index;

    uint32_t uid;
} YAEventHeader;

/**
 *
 * Move:
 *
 * lparam is x point,
 * rparam is y point.
 *
 * Click:
 *
 * lparam:
 *
 * 0 Left,
 * 1 Right,
 * 2 Middle,
 *
 * rparam:
 *
 * 0 is Press
 * 1 is Release
 * 2 is Click
 * 3 is Double click
 *
 * Mouse Wheel:
 * lparam is distance.
 * rparam is direction: 0 up, 1 down, 2 left, 3 right
 *
 * Power/Restart/Hibernate:
 * lparam
 * 1 is Poweroff
 * 2 is Restart
 * 3 is Hibernate
 * rparam no need
 *
 *
 * */
typedef struct
{
    int32_t lparam;
    int32_t rparam;
} YACommonEventRequest;


// Mods bitmask for KEYBOARD_CHORD
#define CHORD_MOD_SHIFT   (1u << 0)
#define CHORD_MOD_CTRL    (1u << 1)
#define CHORD_MOD_ALT     (1u << 2)
#define CHORD_MOD_META    (1u << 3)
#define CHORD_MOD_FN      (1u << 4)

#define CONTROL_OP_SHUTDOWN 1
#define CONTROL_OP_RESTART  2
#define CONTROL_OP_SLEEP    3

/**
 * Keyboard:
 * struct YAKeyboardEventRequest { code, op, mods }
 * - code: 基键（ASCII 0x20..0x7E 或 YA 协议码）
 * - op: 0 Press, 1 Release, 2 Click
 * - mods: 修饰位掩码（见下）
 *
 * 修饰位掩码（mods）：
 * bit0 Shift, bit1 Control, bit2 Alt, bit3 Meta, bit4 Fn
 */
typedef struct
{
    int32_t code;   // 基键（ASCII 或 YA 协议码）
    int32_t op;     // 0 Press, 1 Release, 2 Click
    uint32_t mods;  // 修饰位掩码（bit0 Shift, bit1 Ctrl, bit2 Alt, bit3 Meta, bit4 Fn）
} YAKeyboardEventRequest;

typedef struct
{
    char *text;
} YATextInputEventRequest;

typedef enum
{
    CLIENT_IOS = 0x1,
    CLIENT_ANDROID = 0x2
} YAClientType;

typedef struct
{
    YAClientType type;
    uint32_t version;
} YAAuthorizeEventRequest;

typedef struct
{
    bool success;
    uint32_t os_type; 
    uint32_t uid;
    uint16_t session_port;
    uint16_t command_port;
    char *macs;
    float display_scale; // 主显示器逻辑缩放因子（例如 1.0、1.25、2.0）
    uint32_t client_version; // 客户端协议版本号，用于响应序列化兼容判断
} YAAuthorizeEventResponse;

typedef struct
{
    float pointer_scale;
} YASessionOptionEventRequest;

// 服务器操作系统类型
typedef enum
{
    OS_WINDOWS = 0x1,
    OS_MAC = 0x2,
    OS_LINUX = 0x3,
} YAServerOSType;


typedef struct
{
    char *text;
} YAInputGetEventResponse;

typedef struct
{
    uint32_t magic;
} YADiscoverEventRequest;

typedef struct
{
    uint32_t magic;
    uint16_t port;
} YADiscoverEventResponse;

typedef struct
{
    YAEventHeader header;

    void *param;

    size_t param_len;
} YAEvent;

typedef struct
{
    uint32_t totalSize;
    uint32_t headerSize;
    uint32_t bodySize;
} YAPackageSize;

/**
 * get package size
 * @buffer a buffer
 * @info a struct, package info will be set into this struct
 **/
int ya_get_package_size(struct evbuffer *buffer, YAPackageSize *info);

/**
 * Parse protocol header from buffer
 *
 * @buf received data
 *
 * @out_header out parameter, @See YAEventHeader
 *
 * @length header buffer data length
 *
 * @return -1 is failed, 0 is success, this means the full request/response was
 *received.
 **/
int ya_parse_event_header(struct evbuffer *buf, YAEventHeader *out_header, uint32_t length);

/**
 * Deserialize event from buffer
 * @buf received data
 * @event a pointer of event structure,
 * @size package size
 * you need initialize the event property and this method will fill param's
 *property
 *
 * @return -1 is failed, 0 is success
 **/
int ya_parse_event(struct evbuffer *buf, YAEvent *event, YAPackageSize *size);

/**
 * Serialize an event to buffer
 *
 * @event event
 *
 * @return buffer length
 **/
int ya_serialize_event(YAEvent *event, uint8_t **out);

/**
 * Free all event memory
 * @event event
 **/
void ya_free_event(YAEvent *event);

/**
 * Free event param memory
 * @event event
 **/
void ya_free_event_param(YAEvent *event);
