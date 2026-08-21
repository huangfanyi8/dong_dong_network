// 目标02: 阻塞式单对单 echo client（封装为 EchoClient 类）
//
// 流程: socket -> connect -> send 消息 -> recv 回显 -> close

#include "dong_dong/Platform.h"

#include <cstdint>
#include <cstdio>
#include <cstring>

using dong_dong::SocketFd;

namespace {

class EchoClient
{
public:
    EchoClient()
        : sockFd_(dong_dong::invalidSocket()) {
    }

    ~EchoClient() {
        closeAll();
    }

    // socket + connect：连接服务器
    bool connect(const char* ip, uint16_t port) {
        // 1. 创建 socket
        sockFd_ = ::socket(AF_INET, SOCK_STREAM, 0);
        if (!dong_dong::isValidSocket(sockFd_)) {
            dong_dong::printError("socket()");
            return false;
        }

        // 2. 指定服务器地址
        struct sockaddr_in addr;
        std::memset(&addr, 0, sizeof(addr));
        addr.sin_family = AF_INET;
        addr.sin_addr.s_addr = ::inet_addr(ip);
        addr.sin_port = ::htons(port);

        // 3. connect：主动连接
        if (::connect(sockFd_, reinterpret_cast<struct sockaddr*>(&addr), sizeof(addr)) < 0) {
            dong_dong::printError("connect()");
            return false;
        }
        std::printf("connected to server\n");
        return true;
    }

    // 发一条消息并接收回显
    void echo(const char* msg) {
        // 4. 发送
        ::send(sockFd_, msg, static_cast<int>(std::strlen(msg)), 0);
        std::printf("sent: %s", msg);

        // 5. 接收回显
        char buf[1024];
        int n = ::recv(sockFd_, buf, sizeof(buf) - 1, 0);
        if (n > 0) {
            buf[n] = '\0';
            std::printf("echo: %s", buf);
        } else if (n == 0) {
            std::printf("server closed connection\n");
        } else {
            dong_dong::printError("recv()");
        }
    }

private:
    void closeAll() {
        if (dong_dong::isValidSocket(sockFd_)) {
            dong_dong::closeSocket(sockFd_);
            sockFd_ = dong_dong::invalidSocket();
        }
    }

private:
    SocketFd sockFd_;
};

} // namespace

int main() {
    dong_dong::SocketInitializer init;

    EchoClient client;
    if (!client.connect("127.0.0.1", 8888)) {
        return -1;
    }
    client.echo("hello from client\n");
    return 0;
}
