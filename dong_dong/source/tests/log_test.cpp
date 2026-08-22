// 日志系统独立测试：验证三级别 + socket 错误记录 + 文件/控制台输出 + 文件/行号定位

#include "dong_dong/Logging.h"

void triggerConnectFailure() {
    // 手动触发一次系统错误码：用一个不可能的地址端口触发 connect 失败
    dong_dong::logSocketError("connect", dong_dong::Error::kConnectFailed);
}

void testWarning() {
    dong_dong::logWarning("this is a warning message");
}

void testError() {
    dong_dong::logError("this is an error message");
}

int main() {
    dong_dong::initLogger();

    dong_dong::logInfo("this is an info message");

    testWarning();
    testError();
    triggerConnectFailure();
    return 0;
}
