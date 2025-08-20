
#pragma once
#include <string_view>
#include <string>
#include <cstdint>

namespace logger {

enum class LogLevel : uint8_t {
    Error   = 0,
    Warning = 1,
    Info    = 2
};

inline std::string_view to_string(LogLevel lvl) noexcept {
    switch (lvl) {
        case LogLevel::Error:   return "ERROR";
        case LogLevel::Warning: return "WARN";
        case LogLevel::Info:    return "INFO";
    }
    return "INFO";
}

inline bool parse_level(std::string_view s, LogLevel& out) noexcept {
    if (s == "ERROR" || s == "error" || s == "err" ) { out = LogLevel::Error; return true; }
    if (s == "WARN"  || s == "warning" || s == "warn" ) { out = LogLevel::Warning; return true; }
    if (s == "INFO"  || s == "info"  ) { out = LogLevel::Info; return true; }
    return false;
}

} // namespace logger
