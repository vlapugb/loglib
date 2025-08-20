#include "logger/logger.hpp"
#include "logger/file_sink.hpp"
#include "logger/log_entry.hpp"
#include "logger/socket_sink.hpp"
#include <utility>

namespace logger {
Logger::Logger (std::unique_ptr<ILogSink> sink, const LogLevel default_level) noexcept
: _sink (std::move (sink)), _default (default_level) {
}

Status Logger::log (LogLevel level, const std::string_view msg) noexcept {
    if (static_cast<int> (level) > static_cast<int> (_default.load ())) {
        return Status::Filtered;
    }
    LogEntry e;
    e.epoch_ms = now_epoch_ms ();
    e.level    = level;
    e.message.assign (msg.begin (), msg.end ());
    if (std::string err; !_sink || !_sink->write (e, err)) {
        std::lock_guard lk (_mu);
        _last_err = err.empty () ? "Unknown sink error" : err;
        return Status::IoError;
    }
    return Status::Ok;
}

std::string Logger::last_error () const {
    std::lock_guard lk (_mu);
    return _last_err;
}

void Logger::flush () const noexcept {
    _sink->flush ();
}

std::unique_ptr<ILogSink> make_file_sink (const std::string& path) noexcept {
    return std::make_unique<FileSink> (path);
}

std::unique_ptr<ILogSink> make_socket_sink (const std::string& host, std::uint16_t port) noexcept {
    return std::make_unique<SocketSink> (host, port);
}
} // namespace logger
