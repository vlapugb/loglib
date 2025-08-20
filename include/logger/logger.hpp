
#pragma once
#include "log_sink.hpp"
#include "log_level.hpp"
#include "utils.hpp"
#include <memory>
#include <mutex>
#include <atomic>

namespace logger {

enum class Status {
    Ok,
    Filtered,
    IoError
};

class Logger {
public:
    Logger(std::unique_ptr<ILogSink> sink, LogLevel default_level) noexcept;
    ~Logger() = default;

    Status log(LogLevel level, std::string_view msg) noexcept;

    Status log(const std::string_view msg) noexcept { return log(_default.load(), msg); }

    void set_default_level(const LogLevel lvl) noexcept { _default.store(lvl); }
    LogLevel default_level() const noexcept { return _default.load(); }

    std::string last_error() const;

    void flush() const noexcept;

private:
    std::unique_ptr<ILogSink> _sink;
    std::atomic<LogLevel> _default;
    mutable std::mutex _mu;
    std::string _last_err;
};

std::unique_ptr<ILogSink> make_file_sink(const std::string& path) noexcept;
std::unique_ptr<ILogSink> make_socket_sink(const std::string& host, std::uint16_t port) noexcept;

} // namespace logger
