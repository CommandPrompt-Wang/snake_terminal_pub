#pragma once

#include <cstdarg>
#include <string>

enum class LogLevel {
    Debug,
    Info,
    Warning,
    Error,
};

// 线程安全的日志输出（带颜色）
// module: "Game" | "Raylib" | ...
void log(LogLevel level, const std::string& module, const std::string& msg);

// 接管 raylib 的 TraceLog 输出
void raylib_log_callback(int logLevel, const char* text, va_list args);

// 便捷重载：默认模块 "Game"
inline void log(LogLevel level, const std::string& msg) {
    log(level, "Game", msg);
}
inline void log(const std::string& msg) {
    log(LogLevel::Info, "Game", msg);
}
inline void logd(const std::string& msg) {
    log(LogLevel::Debug, "Game", msg);
}
inline void logw(const std::string& msg) {
    log(LogLevel::Warning, "Game", msg);
}
inline void loge(const std::string& msg) {
    log(LogLevel::Error, "Game", msg);
}
