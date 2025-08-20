
#pragma once
#include "log_level.hpp"
#include <string>
#include <cstdint>

namespace logger {

struct LogEntry {
    std::uint64_t epoch_ms{0};
    LogLevel level{LogLevel::Info};
    std::string message;
};

} // namespace logger
