#pragma once
/**
 * @file
 * @brief Rolling log statistics (totals and last-hour window).
 */

#include "logger/log_level.hpp"
#include <cstddef>
#include <cstdint>
#include <deque>
#include <mutex>

namespace logger {
/**
 * @brief Snapshot of aggregated metrics.
 * @details Arrays use index order: [ERROR, WARN, INFO].
 */
struct StatsSnapshot {
    /** @brief Total records seen since start. */
    std::uint64_t total{ 0 };
    /** @brief Totals by level. */
    std::uint64_t by_level[3]{ 0, 0, 0 };
    /** @brief Minimum message length. */
    std::size_t min_len{ static_cast<std::size_t> (-1) };
    /** @brief Maximum message length. */
    std::size_t max_len{ 0 };
    /** @brief Average message length. */
    double avg_len{ 0.0 };

    /** @brief Records seen within the last hour (rolling window). */
    std::uint64_t last_hour_total{ 0 };
    /** @brief Last-hour totals by level. */
    std::uint64_t last_hour_by_level[3]{ 0, 0, 0 };
    /** @brief Last-hour average message length. */
    double last_hour_avg_len{ 0.0 };
};

/**
 * @brief Thread-safe collector of log stats with a 1-hour sliding window.
 */
class StatsCollector {
    public:
    /**
     * @brief Add one record.
     * @param epoch_ms Timestamp in ms since Unix epoch (UTC).
     * @param lvl Log level.
     * @param msg_len Message length in bytes.
     */
    void add (std::uint64_t epoch_ms, LogLevel lvl, std::size_t msg_len) noexcept;

    /**
     * @brief Compute a snapshot for the given "now".
     * @param now_ms Current time in ms since Unix epoch.
     * @return Aggregated statistics at this moment.
     */
    StatsSnapshot snapshot (std::uint64_t now_ms) noexcept;

    private:
    struct Entry {
        std::uint64_t epoch_ms;
        LogLevel lvl;
        std::size_t len;
    };

    // Cumulative totals (printed statistics)
    std::uint64_t _total{ 0 };
    std::uint64_t _by_level[3]{ 0, 0, 0 };
    std::size_t _min_len{ static_cast<std::size_t> (-1) };
    std::size_t _max_len{ 0 };
    long double _sum_len{ 0.0L };

    std::deque<Entry> _window;
    std::uint64_t _win_total{ 0 };
    std::uint64_t _win_by_level[3]{ 0, 0, 0 };
    long double _win_sum_len{ 0.0L };

    mutable std::mutex _mu;

    /** @brief Drop entries older than @p cutoff_ms from the window. */
    void prune_older_than (std::uint64_t cutoff_ms) noexcept;

    /** @brief Map level to index: ERROR=0, WARN=1, INFO=2. */
    static int idx (const LogLevel l) noexcept {
        switch (l) {
        case LogLevel::Error: return 0;
        case LogLevel::Warning: return 1;
        case LogLevel::Info: return 2;
        }
        return 2;
    }
};
} // namespace logger
