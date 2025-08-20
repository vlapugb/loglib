
#pragma once
#include "log_sink.hpp"
#include <string>
#include <mutex>
#include <cstdint>

namespace logger {

class SocketSink final : public ILogSink {
public:
    SocketSink(std::string  host, std::uint16_t port) noexcept;
    ~SocketSink() override;
    bool write(const LogEntry& e, std::string& err) noexcept override;
    void flush() noexcept override {}

private:
    int _fd{-1};
    std::string _host;
    std::uint16_t _port{0};
    std::mutex _mu;

    bool connect_socket(std::string& err) noexcept;
    void close_socket() noexcept;
};

} // namespace logger
