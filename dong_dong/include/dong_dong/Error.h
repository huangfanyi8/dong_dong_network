#pragma once

namespace dong_dong {

// 网络操作的统一错误码（跨平台，屏蔽 errno / WSAGetLastError 的差异）
enum class Error {
    kSuccess = 0,   // 成功
    kInitFailed,    // 初始化失败（如 WSAStartup）
    kSocketFailed,  // socket() 创建失败
    kBindFailed,    // bind() 绑定失败
    kListenFailed,  // listen() 监听失败
    kAcceptFailed,  // accept() 接收连接失败
    kConnectFailed, // connect() 连接失败
    kSendFailed,    // send() 发送失败
    kRecvFailed,    // recv() 接收失败
    kClosed,        // 对端关闭连接（recv 返回 0）
    kInvalidFd,     // fd 无效
};

// 错误码 → 可读字符串
inline const char* errorToString(Error e) {
    switch (e) {
        case Error::kSuccess:       return "success";
        case Error::kInitFailed:    return "init failed";
        case Error::kSocketFailed:  return "socket() failed";
        case Error::kBindFailed:    return "bind() failed";
        case Error::kListenFailed:  return "listen() failed";
        case Error::kAcceptFailed:  return "accept() failed";
        case Error::kConnectFailed: return "connect() failed";
        case Error::kSendFailed:    return "send() failed";
        case Error::kRecvFailed:    return "recv() failed";
        case Error::kClosed:        return "peer closed connection";
        case Error::kInvalidFd:     return "invalid fd";
    }
    return "unknown error";
}

}
