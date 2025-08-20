
#pragma once
#include "log_entry.hpp"
#include <string>

namespace logger {

class ILogSink {
public:
    virtual ~ILogSink() = default;
    virtual bool write(const LogEntry& e, std::string& err) noexcept = 0;
    virtual void flush() noexcept {}
};

} // namespace logger
