#include "dong_dong/Logging.h"

#include "dong_dong/Platform.h"

#include <spdlog/sinks/daily_file_sink.h>
#include <spdlog/sinks/stdout_color_sinks.h>
#include <spdlog/spdlog.h>

#include <filesystem>
#include <memory>
#include <mutex>

namespace dong_dong {

namespace {

std::mutex g_mutex;
std::shared_ptr<spdlog::logger> g_logger;

// 当前日期字符串，用于日志文件名：YYYY-MM-DD
std::string todayString() {
    auto now = std::chrono::system_clock::now();
    std::time_t t = std::chrono::system_clock::to_time_t(now);
    std::tm tm;
#ifdef _WIN32
    localtime_s(&tm, &t);
#else
    localtime_r(&t, &tm);
#endif
    char buf[16];
    std::snprintf(buf, sizeof(buf), "%04d-%02d-%02d",
                  static_cast<int>(tm.tm_year) + 1900,
                  static_cast<int>(tm.tm_mon) + 1,
                  static_cast<int>(tm.tm_mday));
    return buf;
}

// 把 std::source_location 转成 spdlog 的 source_loc，供日志宏/函数使用
spdlog::source_loc toSpdlogLoc(const std::source_location& loc) {
    return spdlog::source_loc{loc.file_name(),
                              static_cast<int>(loc.line()),
                              loc.function_name()};
}

} // namespace

void initLogger() {
    std::lock_guard<std::mutex> lock(g_mutex);
    if (g_logger) {
        return;
    }

    // 确保 log/ 目录存在
    std::filesystem::create_directories("log");

    // 运行前检查当天日志文件：不存在则创建（daily_file_sink 构造时会打开/创建）
    // daily_file_sink 会生成 log/dong_dong_YYYY-MM-DD.log，并按天自动切换
    auto fileSink = std::make_shared<spdlog::sinks::daily_file_sink_mt>("log/dong_dong.log", 0, 0);
    fileSink->set_level(spdlog::level::info);
    fileSink->set_pattern("[%Y-%m-%d %H:%M:%S.%e][%l][%s:%#][%!] %v");

    auto consoleSink = std::make_shared<spdlog::sinks::stdout_color_sink_mt>();
    consoleSink->set_level(spdlog::level::info);
    consoleSink->set_pattern("[%Y-%m-%d %H:%M:%S.%e][%l][%s:%#][%!] %v");

    g_logger = std::make_shared<spdlog::logger>("dong_dong",
                                                spdlog::sinks_init_list{consoleSink, fileSink});
    g_logger->set_level(spdlog::level::info);

    spdlog::set_default_logger(g_logger);
    SPDLOG_LOGGER_INFO(g_logger, "logger initialized, log file: log/dong_dong_{}.log", todayString());
}

void logInfo(std::string_view msg, std::source_location loc) {
    initLogger();
    g_logger->log(toSpdlogLoc(loc), spdlog::level::info, "{}", msg);
}

void logWarning(std::string_view msg, std::source_location loc) {
    initLogger();
    g_logger->log(toSpdlogLoc(loc), spdlog::level::warn, "{}", msg);
}

void logError(std::string_view msg, std::source_location loc) {
    initLogger();
    g_logger->log(toSpdlogLoc(loc), spdlog::level::err, "{}", msg);
}

void logSocketError(const char* op, Error err, std::source_location loc) {
    initLogger();
    int sysErr = lastError();
    g_logger->log(toSpdlogLoc(loc), spdlog::level::err,
                  "{} error={}, sys_errno={}: {}", op, errorToString(err), sysErr,
                  errorString(sysErr));
}

}
