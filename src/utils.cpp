#include "logger/utils.hpp"

#include <cctype>
#include <chrono>
#include <cstdint>
#include <cstdio>
#include <ctime>
#include <string>
#include <string_view>

namespace logger {
std::uint64_t now_epoch_ms () noexcept {
    using namespace std::chrono;
    return duration_cast<milliseconds> (system_clock::now ().time_since_epoch ()).count ();
}

std::string iso8601_utc (const std::uint64_t epoch_ms) noexcept {
    const auto tt = static_cast<std::time_t> (epoch_ms / 1000ull);
    std::tm tm{};
    if (gmtime_r (&tt, &tm) == nullptr)
        return "1970-01-01T00:00:00Z";
    char buf[21];
    const int n = std::snprintf (buf, sizeof (buf), "%04d-%02d-%02dT%02d:%02d:%02dZ", tm.tm_year + 1900, tm.tm_mon + 1,
    tm.tm_mday, tm.tm_hour, tm.tm_min, tm.tm_sec);
    if (n != 20)
        return "1970-01-01T00:00:00Z";
    return std::string{ buf, 20 };
}

bool split_host_port (std::string_view in, std::string& host, std::uint16_t& port) noexcept {
    const std::size_t p = in.find (':');
    if (p == std::string_view::npos)
        return false;
    if (in.find (':', p + 1) != std::string_view::npos)
        return false;
    const std::string_view h  = in.substr (0, p);
    const std::string_view ps = in.substr (p + 1);
    if (h.empty () || ps.empty ())
        return false;
    unsigned long v = 0;
    for (const char c : ps) {
        if (!std::isdigit (static_cast<unsigned char> (c)))
            return false;
        v = v * 10ul + static_cast<unsigned long> (c - '0');
        if (v > 65535ul)
            return false;
    }
    if (v == 0ul)
        return false;
    host.assign (h.begin (), h.end ());
    port = static_cast<std::uint16_t> (v);
    return true;
}
} // namespace logger
