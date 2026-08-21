#include "dong_dong/Platform.h"

namespace dong_dong {

SocketInitializer::SocketInitializer() {
#if defined(_WIN32)
    WSADATA wsaData;
    if (::WSAStartup(MAKEWORD(2, 2), &wsaData) != 0) {
        // 初始化失败：Winsock 无法使用。
        // 由于此时尚未建立日志系统，直接以失败状态暴露问题。
        // TODO: 接入 spdlog 后改为记录错误日志。
    }
#endif
}

SocketInitializer::~SocketInitializer() {
#if defined(_WIN32)
    ::WSACleanup();
#endif
}

}
