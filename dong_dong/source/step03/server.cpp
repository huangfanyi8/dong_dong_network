// 目标03: 每连接一线程的 echo server
//
// 承接痛点: 目标02 单线程阻塞，一次只能服务一个客户端。
// 方案: 主线程只负责 accept，每接到一个连接就 new 一个 std::thread 去处理，
//       主线程立刻回到 accept 等下一个，从而能同时服务多个客户端。
//
// 注意: 此处刻意用 detach 让线程独立运行，先"能用"；线程管理与资源释放的问题
//       正是本目标的理解重点（目标 03 不做线程池，只感受瓶颈）。

#include "dong_dong/Logging.h"
#include "dong_dong/Platform.h"

#include <cstdint>
#include <cstring>
#include <string>
#include <thread>

using dong_dong::SocketFd;

namespace {

// 处理单个连接的回显循环，在独立线程中运行
void echoLoop(SocketFd connFd) {
    dong_dong::logInfo("handling connection");
    char buf[1024];
    while (true) {
        int n = ::recv(connFd, buf, sizeof(buf) - 1, 0);
        if (n == 0) {
            dong_dong::logInfo("client closed connection");
            break;
        } else if (n < 0) {
            dong_dong::logSocketError("recv", dong_dong::Error::kRecvFailed);
            break;
        }
        buf[n] = '\0';
        dong_dong::logInfo(std::string("received: ") + buf);
        ::send(connFd, buf, n, 0); // 原样回显
    }
    dong_dong::closeSocket(connFd);
}

} // namespace

int main() {
    dong_dong::SocketInitializer init;
    dong_dong::initLogger();

    // 1. socket
    SocketFd listenFd = ::socket(AF_INET, SOCK_STREAM, 0);
    if (!dong_dong::isValidSocket(listenFd)) {
        dong_dong::logSocketError("socket", dong_dong::Error::kSocketFailed);
        return -1;
    }

    // 2. bind
    struct sockaddr_in addr;
    std::memset(&addr, 0, sizeof(addr));
    addr.sin_family = AF_INET;
    addr.sin_addr.s_addr = ::htonl(INADDR_ANY);
    addr.sin_port = ::htons(8888);
    if (::bind(listenFd, reinterpret_cast<struct sockaddr*>(&addr), sizeof(addr)) < 0) {
        dong_dong::logSocketError("bind", dong_dong::Error::kBindFailed);
        return -1;
    }

    // 3. listen
    if (::listen(listenFd, 5) < 0) {
        dong_dong::logSocketError("listen", dong_dong::Error::kListenFailed);
        return -1;
    }
    dong_dong::logInfo("server listening on 8888...");

    // 4. 主循环：一直 accept，每来一个连接就开一个线程
    while (true) {
        struct sockaddr_in peer;
        socklen_t len = sizeof(peer);
        SocketFd connFd = ::accept(listenFd, reinterpret_cast<struct sockaddr*>(&peer), &len);
        if (!dong_dong::isValidSocket(connFd)) {
            dong_dong::logSocketError("accept", dong_dong::Error::kAcceptFailed);
            continue;
        }
        dong_dong::logInfo("accepted a connection, spawning thread...");

        // 5. 每个连接一个线程处理；主线程立刻回来 accept
        std::thread t(echoLoop, connFd);
        t.detach(); // 线程独立运行，主线程不等待
    }

    // 不会走到这里
    dong_dong::closeSocket(listenFd);
    return 0;
}
