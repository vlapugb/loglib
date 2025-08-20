
#pragma once
#include "log_sink.hpp"
#include <fstream>
#include <mutex>

namespace logger {

class FileSink final : public ILogSink {
public:
    explicit FileSink(const std::string& path) noexcept;
    ~FileSink() override;
    bool write(const LogEntry& e, std::string& err) noexcept override;
    void flush() noexcept override;
    bool is_open() const noexcept { return _ofs.is_open(); }
private:
    std::ofstream _ofs;
    std::mutex _mu;
};

}
