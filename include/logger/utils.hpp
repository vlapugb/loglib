
#pragma once
#include <string>
#include <string_view>
#include <cstdint>

namespace logger {

std::uint64_t now_epoch_ms() noexcept;
std::string iso8601_utc(std::uint64_t epoch_ms) noexcept;
bool split_host_port(std::string_view in, std::string& host, std::uint16_t& port) noexcept;

} // namespace logger
