#pragma once
/**
 * @file
 * @brief Log level enum and small helpers.
 */

#include <string_view>
#include <cstdint>

namespace logger {
    /**
     * @brief Severity levels (low index = higher severity).
     */
    enum class LogLevel : uint8_t {
        Error = 0, ///< Fatal/error conditions.
        Warning = 1, ///< Recoverable/attention needed.
        Info = 2 ///< Informational messages.
    };

    /**
     * @brief Canonical upper-case name for a level.
     * @return "ERROR", "WARN" or "INFO".
     */
    inline std::string_view to_string(const LogLevel lvl) noexcept {
        switch (lvl) {
            case LogLevel::Error: return "ERROR";
            case LogLevel::Warning: return "WARN";
            case LogLevel::Info: return "INFO";
        }
        return "INFO";
    }

    /**
     * @brief Parse level from text.
     * @param s Input (accepts ERROR/ERR, WARN/WARNING, INFO; case-insensitive).
     * @param out Parsed level on success.
     * @return true if recognized, false otherwise.
     */
    inline bool parse_level(const std::string_view s, LogLevel &out) noexcept {
        if (s == "ERROR" || s == "error" || s == "err") {
            out = LogLevel::Error;
            return true;
        }
        if (s == "WARN" || s == "warning" || s == "warn") {
            out = LogLevel::Warning;
            return true;
        }
        if (s == "INFO" || s == "info") {
            out = LogLevel::Info;
            return true;
        }
        return false;
    }
}
