// 目标03: 多客户端并发测试程序
//
// 用途: 同时开 N 个客户端线程，每个都连接 server、发送自己的编号消息、接收回显。
// 用于验证目标 03 的 server 能同时服务多个客户端（对比目标 02 只能服务一个）。

#include "dong_dong/Logging.h"
#include "dong_dong/Platform.h"

#include <cstdlib>
#include <cstring>
#include <string>
#include <thread>
#include <vector>

using dong_dong::SocketFd;

namespace {

void runClient(int id) {
    // 每个客户端独立创建自己的 socket，互不共享
    SocketFd sock = ::socket(AF_INET, SOCK_STREAM, 0);
    if (!dong_dong::isValidSocket(sock)) {
        dong_dong::logSocketError("socket", dong_dong::Error::kSocketFailed);
        return;
    }

    struct sockaddr_in addr;
    std::memset(&addr, 0, sizeof(addr));
    addr.sin_family = AF_INET;
    addr.sin_addr.s_addr = ::inet_addr("127.0.0.1");
    addr.sin_port = ::htons(8888);

    if (::connect(sock, reinterpret_cast<struct sockaddr*>(&addr), sizeof(addr)) < 0) {
        dong_dong::logSocketError("connect", dong_dong::Error::kConnectFailed);
        dong_dong::closeSocket(sock);
        return;
    }
    dong_dong::logInfo("client " + std::to_string(id) + " connected");

    // 发送自己的编号消息
    char msg[64];
    std::snprintf(msg, sizeof(msg), "hello from client %d\n", id);
    ::send(sock, msg, static_cast<int>(std::strlen(msg)), 0);

    // 接收回显
    char buf[1024];
    int n = ::recv(sock, buf, sizeof(buf) - 1, 0);
    if (n > 0) {
        buf[n] = '\0';
        dong_dong::logInfo("client " + std::to_string(id) + " echo: " + buf);
    }

    dong_dong::closeSocket(sock);
}

} // namespace

int main(int argc, char** argv) {
    dong_dong::SocketInitializer init;
    dong_dong::initLogger();

    int count = 5; // 默认并发 5 个客户端
    if (argc > 1) {
        count = std::atoi(argv[1]);
    }

    dong_dong::logInfo("spawning " + std::to_string(count) + " clients...");
    std::vector<std::thread> threads;
    for (int i = 0; i < count; ++i) {
        threads.emplace_back(runClient, i);
    }
    for (auto& t : threads) {
        t.join();
    }
    dong_dong::logInfo("all clients done");
    return 0;
}
