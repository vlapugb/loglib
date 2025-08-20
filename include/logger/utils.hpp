#pragma once
/**
 * @file
 * @brief Time and parsing utilities.
 */

#include <cstdint>
#include <string>
#include <string_view>

namespace logger {
/**
 * @brief Get current UTC time in milliseconds since Unix epoch.
 * @return Epoch milliseconds.
 */
std::uint64_t now_epoch_ms () noexcept;

/**
 * @brief Format timestamp as ISO-8601 UTC.
 * @param epoch_ms Milliseconds since Unix epoch (UTC).
 * @return String like "YYYY-MM-DDTHH:MM:SSZ".
 */
std::string iso8601_utc (std::uint64_t epoch_ms) noexcept;

/**
 * @brief Parse "host:port" into parts.
 * @param in Input string (e.g., "example.com:8080").
 * @param host Output host (may be empty if input host is empty).
 * @param port Output TCP port (0..65535).
 * @return true on success, false on parse/range error.
 */
bool split_host_port (std::string_view in, std::string& host, std::uint16_t& port) noexcept;
} // namespace logger
