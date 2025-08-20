#include "logger/file_sink.hpp"
#include "logger/log_level.hpp"
#include "logger/utils.hpp"
#include <iostream>

namespace logger {

FileSink::FileSink (const std::string& path) noexcept {
    _ofs.open (path, std::ios::out | std::ios::app);
}

FileSink::~FileSink () = default;

bool FileSink::write (const LogEntry& e, std::string& err) noexcept {
    std::lock_guard lk (_mu);
    if (!_ofs.is_open ()) {
        err = "FileSink: log file is not open";
        return false;
    }
    _ofs << iso8601_utc (e.epoch_ms) << ' ' << std::string (to_string (e.level)) << ' ' << e.message << '\n';
    if (!_ofs) {
        err = "FileSink: write failed";
        return false;
    }
    return true;
}

void FileSink::flush () noexcept {
    std::lock_guard lk (_mu);
    if (_ofs.is_open ())
        _ofs.flush ();
}

} // namespace logger
