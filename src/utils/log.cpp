#include "utils/log.h"
#include "global.h"

#include <cstdarg>
#include <cstdio>
#include <ctime>
#include <iomanip>
#include <iostream>
#include <mutex>
#include <sstream>

namespace {

// ANSI 颜色码
constexpr const char* RESET  = "\033[0m";
constexpr const char* RED    = "\033[31m";
constexpr const char* YELLOW = "\033[33m";
constexpr const char* GREEN  = "\033[32m";
constexpr const char* CYAN   = "\033[36m";
constexpr const char* WHITE  = "\033[37m";

std::mutex g_log_mutex;

const char* level_to_string(LogLevel level) {
    switch (level) {
    case LogLevel::Debug:   return "DEBUG";
    case LogLevel::Info:    return "INFO";
    case LogLevel::Warning: return "WARN";
    case LogLevel::Error:   return "ERROR";
    }
    return "????";
}

const char* level_to_color(LogLevel level) {
    switch (level) {
    case LogLevel::Debug:   return CYAN;
    case LogLevel::Info:    return GREEN;
    case LogLevel::Warning: return YELLOW;
    case LogLevel::Error:   return RED;
    }
    return WHITE;
}

} // namespace

void log(LogLevel level, const std::string& module, const std::string& msg) {
    if (level < Global::loglevel) return;

    std::lock_guard<std::mutex> lock(g_log_mutex);

    auto t  = std::time(nullptr);
    std::tm tm{};
#if defined(_WIN32)
    localtime_s(&tm, &t);
#else
    localtime_r(&t, &tm);
#endif

    std::ostringstream time_buf;
    time_buf << std::put_time(&tm, "%H:%M:%S");

    std::cerr << level_to_color(level)
              << "[" << level_to_string(level) << "]"
              << "[" << time_buf.str() << "]"
              << RESET
              << "[" << module << "] "
              << msg << std::endl;
}

void raylib_log_callback(int logLevel, const char* text, va_list args) {
    // raylib log levels:
    // LOG_ALL=0, LOG_TRACE=1, LOG_DEBUG=2, LOG_INFO=3,
    // LOG_WARNING=4, LOG_ERROR=5, LOG_FATAL=6, LOG_NONE=7
    if (logLevel <= 0 || logLevel >= 7) return; // LOG_ALL / LOG_NONE → 跳过

    LogLevel level;
    switch (logLevel) {
    case 1: // LOG_TRACE
    case 2: // LOG_DEBUG
        level = LogLevel::Debug; break;
    case 3: // LOG_INFO → Debug（raylib 的 INFO 多为纹理/文件加载细节）
        level = LogLevel::Debug; break;
    case 4: // LOG_WARNING
        level = LogLevel::Warning; break;
    case 5: // LOG_ERROR
    case 6: // LOG_FATAL
        level = LogLevel::Error; break;
    default:
        return;
    }

    if (level < Global::loglevel) return;

    char buffer[1024];
    vsnprintf(buffer, sizeof(buffer), text, args);
    log(level, "Raylib", std::string(buffer));
}
