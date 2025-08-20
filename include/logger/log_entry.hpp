#pragma once
/**
 * @file
 * @brief Basic log record type.
 */

#include "log_level.hpp"
#include <cstdint>
#include <string>

namespace logger {
/**
 * @brief One log entry with timestamp, level, and message.
 */
struct LogEntry {
    /** @brief Timestamp since Unix epoch in milliseconds (UTC). */
    std::uint64_t epoch_ms{ 0 };
    /** @brief Severity level. */
    LogLevel level{ LogLevel::Info };
    /** @brief Message payload (no trailing newline). */
    std::string message;
};
} // namespace logger
