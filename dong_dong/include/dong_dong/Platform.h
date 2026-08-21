#pragma once

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

}
