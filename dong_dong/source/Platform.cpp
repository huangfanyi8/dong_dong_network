#include "dong_dong/Logging.h"
#include "dong_dong/Platform.h"

namespace dong_dong {

SocketInitializer::SocketInitializer() {
#if defined(_WIN32)
    WSADATA wsaData;
    if (::WSAStartup(MAKEWORD(2, 2), &wsaData) != 0) {
        // 初始化失败：Winsock 无法使用。记录错误码以便排查。
        logSocketError("WSAStartup", Error::kInitFailed);
    }
#endif
}

SocketInitializer::~SocketInitializer() {
#if defined(_WIN32)
    ::WSACleanup();
#endif
}

}
