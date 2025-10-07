#include "ya_utils.h"
#include "event2/buffer.h"

#include <errno.h>
#include <math.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>

#ifdef _WIN32
#include <windows.h>
#include <winsock2.h>
#include <ws2tcpip.h>
#include <iphlpapi.h>
#include <fileapi.h>
#include <direct.h>  /* 提供 _mkdir 函数 */
#pragma comment(lib, "ws2_32.lib")
#ifdef _MSC_VER
#pragma comment(lib, "Iphlpapi.lib")
#endif
#define stat _stat
#define MAX_PATH_LEN MAX_PATH
#else
#include <netinet/in.h>
#include <unistd.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <pwd.h>
#ifdef PATH_MAX
#define MAX_PATH_LEN PATH_MAX
#else
#define MAX_PATH_LEN 4096
#endif
#endif

#ifdef __APPLE__
#include <mach-o/dyld.h>
#include <ifaddrs.h>
#include <net/if_dl.h>
#include <net/if.h>
#include <CoreGraphics/CoreGraphics.h>
#endif
 
#if defined(__linux__)
#include <ifaddrs.h>
#include <netpacket/packet.h>
#include <net/if.h>
#endif

void safe_free(void **ptr)
{
    if (ptr && *ptr)
    {
        free(*ptr);
        *ptr = NULL;
    }
}

int ya_get_home_dir(char *out_home, size_t out_size)
{
    if (!out_home || out_size == 0) return -1;
#ifdef _WIN32
    return -1;
#else
    struct passwd *pw = getpwuid(getuid());
    if (pw && pw->pw_dir && pw->pw_dir[0] != '\0')
    {
        strncpy(out_home, pw->pw_dir, out_size);
        out_home[out_size - 1] = '\0';
        return 0;
    }
    return -1;
#endif
}

size_t get_real_path_limit()
{
#ifdef PATH_MAX
    return PATH_MAX;
#elif defined(MAX_PATH)
    return MAX_PATH;
#else
    return 4096; // 默认值
#endif
}

int file_exists(const char *path)
{
    struct stat buffer;

    int maxsize = get_real_path_limit();
    char normalized_path[maxsize];

    strncpy(normalized_path, path, maxsize - 1);
    normalized_path[maxsize - 1] = '\0';

    for (char *p = normalized_path; *p; p++)
    {
        if (*p == '\\')
            *p = '/';
    }

    return (stat(normalized_path, &buffer) == 0);
}

bool ya_is_wayland_session(void)
{
#ifdef __linux__
    const char *xdg_session = getenv("XDG_SESSION_TYPE");
    if (xdg_session && strcmp(xdg_session, "wayland") == 0)
    {
        const char *wayland_display = getenv("WAYLAND_DISPLAY");
        if (wayland_display && *wayland_display)
        {
            return true;
        }
    }
#endif
    return false;
}

uint32_t to_uint32(struct evbuffer *buffer, int start)
{
    uint32_t out = 0;

    struct evbuffer_ptr ptr = {0};
    evbuffer_ptr_set(buffer, &ptr, start, EVBUFFER_PTR_SET);
    evbuffer_copyout_from(buffer, &ptr, &out, sizeof(uint32_t));

    out = ntohl(out);

    return out;
}

void ya_normalize_path(char* path)
{
    if (!path) return;
    size_t len = strlen(path);
    if (len == 0) return;
#ifdef _WIN32
    for (size_t i = 0; i < len; ++i) {
        if (path[i] == '/') path[i] = '\\';
    }
    // 去除结尾多余分隔符
    while (len > 0 && path[len - 1] == '\\') {
        path[len - 1] = '\0';
        --len;
    }
#else
    for (size_t i = 0; i < len; ++i) {
        if (path[i] == '\\') path[i] = '/';
    }
    while (len > 0 && path[len - 1] == '/') {
        path[len - 1] = '\0';
        --len;
    }
#endif
}

int ya_ensure_dir(const char* path)
{
    if (!path || !*path) {
        return 0;
    }

    char tmp[512];
    snprintf(tmp, sizeof(tmp), "%s", path);
    size_t len = strlen(tmp);

    // Remove trailing slashes unless it's the root directory
    if (len > 1 && (tmp[len - 1] == '/' || tmp[len - 1] == '\\')) {
        tmp[len - 1] = '\0';
    }

    // Iterate through the path and create directories
    for (char* p = tmp; *p; p++) {
        if (*p == '/' || *p == '\\') {
            // Skip the root directory separator and Windows drive letters
            if (p == tmp || (p > tmp && *(p - 1) == ':')) {
                continue;
            }
            
            *p = '\0'; // Temporarily terminate the string
#ifdef _WIN32
            if (_mkdir(tmp) != 0 && errno != EEXIST) {
                *p = '\\'; // Restore separator
                return -1;
            }
#else
            if (mkdir(tmp, 0755) != 0 && errno != EEXIST) {
                *p = '/'; // Restore separator
                return -1;
            }
#endif
#ifdef _WIN32
            *p = '\\';
#else
            *p = '/';
#endif
        }
    }

    // Create the final directory
#ifdef _WIN32
    if (_mkdir(tmp) != 0 && errno != EEXIST) {
        return -1;
    }
#else
    if (mkdir(tmp, 0755) != 0 && errno != EEXIST) {
        return -1;
    }
#endif

    return 0;
}

void ya_get_peer_addr(evutil_socket_t fd, char *addr_buf, size_t buf_len) {
    struct sockaddr_storage peer_addr;
    socklen_t peer_len = sizeof(peer_addr);
    if (getpeername(fd, (struct sockaddr *)&peer_addr, &peer_len) == 0) {
        if (peer_addr.ss_family == AF_INET) {
            struct sockaddr_in *s = (struct sockaddr_in *)&peer_addr;
#ifdef _WIN32
            char *addr_str = inet_ntoa(s->sin_addr);
            if (addr_str) {
                strncpy(addr_buf, addr_str, buf_len - 1);
                addr_buf[buf_len - 1] = '\0';
            } else {
                strncpy(addr_buf, "unknown", buf_len - 1);
                addr_buf[buf_len - 1] = '\0';
            }
#else
            inet_ntop(AF_INET, &s->sin_addr, addr_buf, buf_len);
#endif
        }
        
        
//         else if (peer_addr.ss_family == AF_INET6) { // AF_INET6
//             struct sockaddr_in6 *s = (struct sockaddr_in6 *)&peer_addr;
// #ifdef _WIN32
//             strncpy(addr_buf,  &s->sin6_addr, buf_len - 1);
//             addr_buf[buf_len - 1] = '\0';
// #else
//             inet_ntop(AF_INET6, &s->sin6_addr, addr_buf, buf_len);
// #endif
//         }
    } else {
        strncpy(addr_buf, "unknown", buf_len - 1);
        addr_buf[buf_len - 1] = '\0';
    }
}

int ya_get_executable_path(char* path_buf, size_t buf_len) {
#if defined(_WIN32)
    if (GetModuleFileNameA(NULL, path_buf, buf_len) == 0) {
        return -1;
    }
#elif defined(__APPLE__)
    uint32_t size = buf_len;
    if (_NSGetExecutablePath(path_buf, &size) != 0) {
        return -1; // Buffer too small
    }
#else // Linux
    ssize_t len = readlink("/proc/self/exe", path_buf, buf_len - 1);
    if (len == -1) {
        return -1;
    }
    path_buf[len] = '\0';
#endif
    return 0;
}

char* ya_get_dir_path(char* file_path) {
    if (!file_path) return NULL;
    char* last_slash = strrchr(file_path, '/');
    char* last_backslash = strrchr(file_path, '\\');
    char* last_separator = (last_slash > last_backslash) ? last_slash : last_backslash;

    if (last_separator != NULL) {
        *last_separator = '\0';
        return file_path;
    }
    return NULL;
}

static int format_mac_lower_colon(const unsigned char *bytes, size_t len, char *out, size_t out_size)
{
    if (len < 6 || out_size < 18) return -1;
    snprintf(out, out_size, "%02x:%02x:%02x:%02x:%02x:%02x",
             bytes[0], bytes[1], bytes[2], bytes[3], bytes[4], bytes[5]);
    return 0;
}

char* ya_get_macs_csv(void)
{
#if defined(_WIN32)
    // 使用 GetAdaptersAddresses 获取物理地址
    ULONG flags = GAA_FLAG_SKIP_ANYCAST | GAA_FLAG_SKIP_DNS_SERVER | GAA_FLAG_SKIP_FRIENDLY_NAME;
    ULONG family = AF_UNSPEC;
    ULONG outBufLen = 0;
    IP_ADAPTER_ADDRESSES *pAddresses = NULL, *pCurr = NULL;
    DWORD dwRetVal = GetAdaptersAddresses(family, flags, NULL, pAddresses, &outBufLen);
    if (dwRetVal == ERROR_BUFFER_OVERFLOW) {
        pAddresses = (IP_ADAPTER_ADDRESSES *)malloc(outBufLen);
        if (!pAddresses) return NULL;
        dwRetVal = GetAdaptersAddresses(family, flags, NULL, pAddresses, &outBufLen);
    }
    if (dwRetVal != NO_ERROR) {
        if (pAddresses) free(pAddresses);
        return NULL;
    }

    size_t cap = 128;
    size_t len = 0;
    char *buf = (char *)malloc(cap);
    if (!buf) { free(pAddresses); return NULL; }
    buf[0] = '\0';

    char macstr[32];

    for (pCurr = pAddresses; pCurr != NULL; pCurr = pCurr->Next) {
        // 跳过环回与隧道
        if (pCurr->IfType == IF_TYPE_SOFTWARE_LOOPBACK || pCurr->IfType == IF_TYPE_TUNNEL) {
            continue;
        }
        if (pCurr->PhysicalAddressLength == 6) {
            const unsigned char *mac = (const unsigned char *)pCurr->PhysicalAddress;
            int all_zero = 1; for (int i=0;i<6;i++){ if (mac[i]!=0){ all_zero=0; break; } }
            if (all_zero) continue;
            if (format_mac_lower_colon(mac, 6, macstr, sizeof(macstr)) != 0) continue;

            size_t need = strlen(macstr) + 1 + (len > 0 ? 1 : 0);
            if (len + need >= cap) {
                size_t newcap = cap * 2; while (len + need >= newcap) newcap *= 2;
                char *nbuf = (char *)realloc(buf, newcap);
                if (!nbuf) { free(buf); free(pAddresses); return NULL; }
                buf = nbuf; cap = newcap;
            }
            if (len > 0) { buf[len++] = ','; buf[len] = '\0'; }
            strcpy(buf + len, macstr);
            len += strlen(macstr);
        }
    }

    free(pAddresses);
    if (len == 0) { free(buf); return NULL; }
    return buf;
#else
    struct ifaddrs *ifaddr = NULL, *ifa = NULL;
    if (getifaddrs(&ifaddr) == -1) {
        return NULL;
    }

    // 累积到动态缓冲
    size_t cap = 128;
    size_t len = 0;
    char *buf = malloc(cap);
    if (!buf) { freeifaddrs(ifaddr); return NULL; }
    buf[0] = '\0';

    char macstr[32];

    for (ifa = ifaddr; ifa != NULL; ifa = ifa->ifa_next)
    {
        if (!ifa->ifa_addr) continue;
        // 跳过环回
        if (ifa->ifa_flags & IFF_LOOPBACK) continue;

#ifdef __APPLE__
        if (ifa->ifa_addr->sa_family == AF_LINK)
        {
            struct sockaddr_dl *sdl = (struct sockaddr_dl *)ifa->ifa_addr;
            unsigned char *mac = (unsigned char *)LLADDR(sdl);
            if (sdl->sdl_alen != 6) continue;
            // 跳过全0
            int all_zero = 1; for (int i=0;i<6;i++){ if (mac[i]!=0){ all_zero=0; break; } }
            if (all_zero) continue;
            if (format_mac_lower_colon(mac, 6, macstr, sizeof(macstr)) != 0) continue;
        } else {
            continue;
        }
#elif defined(__linux__)
        if (ifa->ifa_addr->sa_family == AF_PACKET)
        {
            struct sockaddr_ll *s = (struct sockaddr_ll *)ifa->ifa_addr;
            if (s->sll_halen != 6) continue;
            unsigned char *mac = (unsigned char *)s->sll_addr;
            int all_zero = 1; for (int i=0;i<6;i++){ if (mac[i]!=0){ all_zero=0; break; } }
            if (all_zero) continue;
            if (format_mac_lower_colon(mac, 6, macstr, sizeof(macstr)) != 0) continue;
        } else {
            continue;
        }
#else
        continue;
#endif

        size_t need = strlen(macstr) + 1 + (len > 0 ? 1 : 0);
        if (len + need >= cap)
        {
            size_t newcap = cap * 2; while (len + need >= newcap) newcap *= 2;
            char *nbuf = realloc(buf, newcap);
            if (!nbuf) { free(buf); freeifaddrs(ifaddr); return NULL; }
            buf = nbuf; cap = newcap;
        }
        if (len > 0) { buf[len++] = ','; buf[len] = '\0'; }
        strcpy(buf + len, macstr);
        len += strlen(macstr);
    }

    freeifaddrs(ifaddr);
    if (len == 0) { free(buf); return NULL; }
    return buf;
#endif
}

// 解析环境变量中的浮点缩放值（>0 有效）
static float ya_parse_scale_env(const char *env_name)
{
    const char *v = getenv(env_name);
    if (!v || !*v)
        return 0.0f;
    char *endp = NULL;
    double d = strtod(v, &endp);
    if (endp == v)
        return 0.0f;
    if (!isfinite(d) || d <= 0.0)
        return 0.0f;
    return (float)d;
}

// 返回主显示器逻辑缩放因子
float ya_primary_scale(void)
{
    // 1) 显式环境变量覆盖
    float s = ya_parse_scale_env("MH_DISPLAY_SCALE");
    if (s > 0.0f)
        return s;

#ifdef __APPLE__
    // 2) macOS: 使用 CoreGraphics 主显示器像素/点比例
    CGDirectDisplayID mainDisplay = CGMainDisplayID();
    CGDisplayModeRef mode = CGDisplayCopyDisplayMode(mainDisplay);
    if (mode)
    {
        size_t w_points = (size_t)CGDisplayModeGetWidth(mode);
        size_t w_pixels = (size_t)CGDisplayModeGetPixelWidth(mode);
        float scale = 1.0f;
        if (w_points > 0)
            scale = (float)w_pixels / (float)w_points;
        CGDisplayModeRelease(mode);
        if (scale > 0.0f && isfinite(scale))
            return scale;
    }
    return 1.0f;
#elif defined(_WIN32)
    // 2) Windows: 使用设备 DPI/96
    HDC hdc = GetDC(NULL);
    if (hdc)
    {
        int dpi = GetDeviceCaps(hdc, LOGPIXELSX);
        ReleaseDC(NULL, hdc);
        if (dpi > 0)
        {
            float scale = (float)dpi / 96.0f;
            if (scale > 0.0f && isfinite(scale))
                return scale;
        }
    }
    return 1.0f;
#else
    // 2) Linux: 读取常见环境变量作为近似
    // GDK: 有两种：GDK_SCALE(整数倍) 与 GDK_DPI_SCALE(浮点微调)，一般总缩放 = GDK_SCALE * GDK_DPI_SCALE
    float gdk_scale = ya_parse_scale_env("GDK_SCALE");
    if (gdk_scale <= 0.0f)
    {
        // 某些环境将 GDK_SCALE 设为整数，如 "2"
        const char *gv = getenv("GDK_SCALE");
        if (gv && *gv)
        {
            long iv = strtol(gv, NULL, 10);
            if (iv > 0)
                gdk_scale = (float)iv;
        }
    }
    float gdk_dpi_scale = ya_parse_scale_env("GDK_DPI_SCALE");
    float qt_scale = ya_parse_scale_env("QT_SCALE_FACTOR");
    float qt_dpr = ya_parse_scale_env("QT_DEVICE_PIXEL_RATIO");

    if (gdk_scale > 0.0f || gdk_dpi_scale > 0.0f)
    {
        float base = (gdk_scale > 0.0f ? gdk_scale : 1.0f);
        float fine = (gdk_dpi_scale > 0.0f ? gdk_dpi_scale : 1.0f);
        float scale = base * fine;
        if (scale > 0.0f && isfinite(scale))
            return scale;
    }
    if (qt_scale > 0.0f)
        return qt_scale;
    if (qt_dpr > 0.0f)
        return qt_dpr;

    return 1.0f;
#endif
}
