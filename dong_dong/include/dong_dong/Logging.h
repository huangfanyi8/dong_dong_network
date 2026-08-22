#pragma once

#include "dong_dong/Error.h"

#include <source_location>
#include <string_view>

namespace dong_dong {

// 初始化日志系统：
//  - 控制台输出（带颜色）
//  - 文件输出：log/dong_dong_YYYY-MM-DD.log（按日期每天一个文件，跨天自动切换）
// 运行前会确保 log/ 目录存在并创建/复用当天日志文件。
void initLogger();

// 三个日志级别。函数式接口，通过默认实参自动捕获调用位置（文件/行号/函数名）。
void logInfo(std::string_view msg,
             std::source_location loc = std::source_location::current());
void logWarning(std::string_view msg,
                std::source_location loc = std::source_location::current());
void logError(std::string_view msg,
              std::source_location loc = std::source_location::current());

// 记录一次 socket 操作失败：操作名 + 统一错误码 + 底层系统错误码。
// 例如 logSocketError("connect", Error::kConnectFailed)
//   → "connect() failed, error=connect() failed, sys_errno=10061: <描述>"
void logSocketError(const char* op, Error err,
                    std::source_location loc = std::source_location::current());

}
