#pragma once
#include <cstdint>
#include <cstddef>
#include <deque>
#include <mutex>
#include "logger/log_level.hpp"

namespace logger {

struct StatsSnapshot {
    std::uint64_t total{0};
    std::uint64_t by_level[3]{0,0,0};
    std::size_t   min_len{static_cast<std::size_t>(-1)};
    std::size_t   max_len{0};
    double        avg_len{0.0};
    std::uint64_t last_hour_total{0};
    std::uint64_t last_hour_by_level[3]{0,0,0};
    double        last_hour_avg_len{0.0};
};

class StatsCollector {
public:
    void add(std::uint64_t epoch_ms, LogLevel lvl, std::size_t msg_len) noexcept;
    StatsSnapshot snapshot(std::uint64_t now_ms) noexcept;
private:
    struct Entry { std::uint64_t epoch_ms; LogLevel lvl; std::size_t len; };
    std::uint64_t _total{0};
    std::uint64_t _by_level[3]{0,0,0};
    std::size_t   _min_len{static_cast<std::size_t>(-1)};
    std::size_t   _max_len{0};
    long double   _sum_len{0.0L};
    std::deque<Entry> _window;
    std::uint64_t _win_total{0};
    std::uint64_t _win_by_level[3]{0,0,0};
    long double   _win_sum_len{0.0L};
    mutable std::mutex _mu;
    void prune_older_than(std::uint64_t cutoff_ms) noexcept;
    static int idx(LogLevel l) noexcept {
        switch(l){ case LogLevel::Error: return 0; case LogLevel::Warning: return 1; case LogLevel::Info: return 2; }
        return 2;
    }
};

} // namespace logger
