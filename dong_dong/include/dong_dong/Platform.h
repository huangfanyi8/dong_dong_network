#pragma once

#include <cerrno>
#include <cstring>

#if defined(_WIN32)
#define WIN32_LEAN_AND_MEAN
#define NOMINMAX
#include <winsock2.h>
#include <ws2tcpip.h>
#else
#include <arpa/inet.h>
#include <netinet/in.h>
#include <sys/socket.h>
#include <unistd.h>
#endif

namespace dong_dong {

// 跨平台 socket 初始化。
// Windows 上必须调用 WSAStartup() 之后才能使用任何 Winsock 函数；
// Linux 上无需任何初始化，此类为空实现。
// 用 RAII 保证 WSAStartup 与 WSACleanup 成对调用，避免遗忘清理。
class SocketInitializer
{
public:
    SocketInitializer();
    ~SocketInitializer();

    SocketInitializer(const SocketInitializer&) = delete;
    SocketInitializer& operator=(const SocketInitializer&) = delete;
};

// 跨平台 socket 句柄类型。
// Windows 是 SOCKET（无符号），Linux 是 int（>=0 为有效），
// 因此"是否有效/小于0"的判断不能直接比较，必须用下面的辅助函数。
#ifdef _WIN32
using SocketFd = SOCKET;
#else
using SocketFd = int;
#endif

// 返回一个无效的 fd 值，用于初始化变量。
inline SocketFd invalidSocket() {
#ifdef _WIN32
    return INVALID_SOCKET;
#else
    return -1;
#endif
}

// 判断 fd 是否有效。
inline bool isValidSocket(SocketFd fd) {
#ifdef _WIN32
    return fd != INVALID_SOCKET;
#else
    return fd >= 0;
#endif
}

// 获取最近一次 socket 调用的错误码。
// Windows 用 WSAGetLastError()，Linux 用 errno。
inline int lastError() {
#ifdef _WIN32
    return ::WSAGetLastError();
#else
    return errno;
#endif
}

// 把错误码转为可读字符串。
// Windows 的 Winsock 错误码用 FormatMessage 转；
// Linux 的 errno 用 strerror 转。
inline const char* errorString(int err) {
#ifdef _WIN32
    static char buf[256];
    ::FormatMessageA(FORMAT_MESSAGE_FROM_SYSTEM | FORMAT_MESSAGE_IGNORE_INSERTS,
                     nullptr, static_cast<DWORD>(err), 0, buf, sizeof(buf), nullptr);
    return buf;
#else
    return ::strerror(err);
#endif
}

// 关闭 socket（跨平台）。
inline void closeSocket(SocketFd fd) {
#ifdef _WIN32
    ::closesocket(fd);
#else
    ::close(fd);
#endif
}

}
