// 目标02: 阻塞式单对单 echo server（封装为 EchoServer 类）
//
// 流程: socket -> bind -> listen -> accept -> recv/send 循环回显 -> close
// 平台差异统一由 dong_dong/Platform.h 处理（类型、错误码、close）。

#include "dong_dong/Logging.h"
#include "dong_dong/Platform.h"

#include <cstdint>
#include <cstring>
#include <string>

using dong_dong::SocketFd;

namespace {

class EchoServer
{
public:
    explicit EchoServer(uint16_t port)
        : port_(port)
        , listenFd_(dong_dong::invalidSocket())
        , connFd_(dong_dong::invalidSocket()) {
    }

    ~EchoServer() {
        closeAll();
    }

    // socket + bind + listen：启动监听
    bool start() {
        // 1. 创建监听 socket（TCP）
        listenFd_ = ::socket(AF_INET, SOCK_STREAM, 0);
        if (!dong_dong::isValidSocket(listenFd_)) {
            dong_dong::logSocketError("socket", dong_dong::Error::kSocketFailed);
            return false;
        }

        // 2. bind 到 0.0.0.0:port（监听所有网卡）
        struct sockaddr_in addr;
        std::memset(&addr, 0, sizeof(addr));
        addr.sin_family = AF_INET;
        addr.sin_addr.s_addr = ::htonl(INADDR_ANY);
        addr.sin_port = ::htons(port_);
        if (::bind(listenFd_, reinterpret_cast<struct sockaddr*>(&addr), sizeof(addr)) < 0) {
            dong_dong::logSocketError("bind", dong_dong::Error::kBindFailed);
            return false;
        }

        // 3. listen：进入监听状态
        if (::listen(listenFd_, 5) < 0) {
            dong_dong::logSocketError("listen", dong_dong::Error::kListenFailed);
            return false;
        }
        dong_dong::logInfo("server listening on " + std::to_string(port_));
        return true;
    }

    // accept + 回显循环：阻塞处理一个客户端
    void serve() {
        // 4. accept：阻塞等待一个客户端连接，返回连接 fd
        struct sockaddr_in peer;
        socklen_t len = sizeof(peer);
        connFd_ = ::accept(listenFd_, reinterpret_cast<struct sockaddr*>(&peer), &len);
        if (!dong_dong::isValidSocket(connFd_)) {
            dong_dong::logSocketError("accept", dong_dong::Error::kAcceptFailed);
            return;
        }
        dong_dong::logInfo("client connected");

        // 5. 回显循环
        char buf[1024];
        while (true) {
            int n = ::recv(connFd_, buf, sizeof(buf) - 1, 0);
            if (n == 0) {
                dong_dong::logInfo("client closed connection");
                break;
            } else if (n < 0) {
                dong_dong::logSocketError("recv", dong_dong::Error::kRecvFailed);
                break;
            }
            buf[n] = '\0';
            dong_dong::logInfo(std::string("received: ") + buf);
            ::send(connFd_, buf, n, 0); // 原样回显
        }
    }

private:
    void closeAll() {
        if (dong_dong::isValidSocket(connFd_)) {
            dong_dong::closeSocket(connFd_);
            connFd_ = dong_dong::invalidSocket();
        }
        if (dong_dong::isValidSocket(listenFd_)) {
            dong_dong::closeSocket(listenFd_);
            listenFd_ = dong_dong::invalidSocket();
        }
    }

private:
    uint16_t port_;
    SocketFd listenFd_; // 监听 fd：负责接收连接
    SocketFd connFd_;   // 连接 fd：负责收发数据
};

} // namespace

int main() {
    dong_dong::SocketInitializer init;
    dong_dong::initLogger();

    EchoServer server(8888);
    if (!server.start()) {
        return -1;
    }
    server.serve();
    return 0;
}
