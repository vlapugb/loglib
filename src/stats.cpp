#include "logger/stats.hpp"
#include "logger/utils.hpp"

namespace logger {
    void StatsCollector::add(const std::uint64_t epoch_ms, const LogLevel lvl, const std::size_t msg_len) noexcept {
        std::lock_guard lk(_mu);
        _total++;
        _by_level[idx(lvl)]++;
        if (_min_len == static_cast<std::size_t>(-1) || msg_len < _min_len) _min_len = msg_len;
        if (msg_len > _max_len) _max_len = msg_len;
        _sum_len += static_cast<long double>(msg_len);
        _window.push_back({epoch_ms, lvl, msg_len});
        _win_total++;
        _win_by_level[idx(lvl)]++;
        _win_sum_len += static_cast<long double>(msg_len);
        prune_older_than(now_epoch_ms() - 3600ull * 1000ull);
    }

    void StatsCollector::prune_older_than(const std::uint64_t cutoff_ms) noexcept {
        while (!_window.empty() && _window.front().epoch_ms < cutoff_ms) {
            const auto e = _window.front();
            _window.pop_front();
            _win_total--;
            _win_by_level[idx(e.lvl)]--;
            _win_sum_len -= static_cast<long double>(e.len);
        }
    }

    StatsSnapshot StatsCollector::snapshot(const std::uint64_t now_ms) noexcept {
        std::lock_guard<std::mutex> lk(_mu);
        prune_older_than(now_ms - 3600ull * 1000ull);
        StatsSnapshot s;
        s.total = _total;
        s.by_level[0] = _by_level[0];
        s.by_level[1] = _by_level[1];
        s.by_level[2] = _by_level[2];
        s.min_len = (_total == 0 ? 0 : _min_len);
        s.max_len = _max_len;
        s.avg_len = (_total == 0 ? 0.0 : static_cast<double>(_sum_len / static_cast<long double>(_total)));
        s.last_hour_total = _win_total;
        s.last_hour_by_level[0] = _win_by_level[0];
        s.last_hour_by_level[1] = _win_by_level[1];
        s.last_hour_by_level[2] = _win_by_level[2];
        s.last_hour_avg_len = (_win_total == 0
                                   ? 0.0
                                   : static_cast<double>(_win_sum_len / static_cast<long double>(_win_total)));
        return s;
    }
} // namespace logger
