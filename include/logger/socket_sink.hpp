#pragma once
/**
 * @file
 * @brief TCP socket log sink.
 */

#include "log_sink.hpp"
#include <string>
#include <mutex>
#include <cstdint>

namespace logger {
    /**
     * @brief Sends log entries to a TCP endpoint (thread-safe).
     */
    class SocketSink final : public ILogSink {
    public:
        /**
         * @brief Create sink for @p host:@p port.
         * @note Never throws; connection is lazy.
         */
        SocketSink(std::string host, std::uint16_t port) noexcept;

        /** @brief Close socket if open. */
        ~SocketSink() override;

        /**
         * @brief Send one entry; connects on first use.
         * @param e Log entry to send.
         * @param err Error text on failure.
         * @return true on success, false on I/O error.
         */
        bool write(const LogEntry &e, std::string &err) noexcept override;

        /** @brief No-op for sockets. */
        void flush() noexcept override {
        }

    private:
        int _fd{-1}; ///< Socket fd or -1 if closed.
        std::string _host; ///< Target host.
        std::uint16_t _port{0}; ///< Target port.
        std::mutex _mu; ///< Guards connect/send/close.

        /**
         * @brief Ensure socket is connected.
         * @return true if connected/ready, false on error (sets @p err).
         */
        bool connect_socket(std::string &err) noexcept;

        /** @brief Close socket fd (if any). */
        void close_socket() noexcept;
    };
}
