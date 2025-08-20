#pragma once
/**
 * @file
 * @brief Core logger API: status codes, Logger facade, and sink factories.
 */

#include "log_level.hpp"
#include "log_sink.hpp"
#include "utils.hpp"
#include <atomic>
#include <memory>
#include <mutex>

namespace logger {
/**
 * @brief Result of a logging attempt.
 */
enum class Status {
    Ok,       ///< Entry accepted by sink.
    Filtered, ///< Dropped by level filter.
    IoError   ///< Sink I/O failure (see @ref Logger::last_error()).
};

/**
 * @brief Thread-safe logger with level filtering and pluggable sink.
 */
class Logger {
    public:
    /**
     * @brief Construct with sink and default level.
     * @param sink Destination (owned).
     * @param default_level Minimum level to pass through.
     */
    Logger (std::unique_ptr<ILogSink> sink, LogLevel default_level) noexcept;

    ~Logger () = default;

    /**
     * @brief Log a message with explicit level.
     * @return @ref Status of the operation.
     */
    Status log (LogLevel level, std::string_view msg) noexcept;

    /**
     * @brief Log with the current default level.
     */
    Status log (const std::string_view msg) noexcept {
        return log (_default.load (), msg);
    }

    /** @brief Set/Get default severity threshold. */
    void set_default_level (const LogLevel lvl) noexcept {
        _default.store (lvl);
    }
    LogLevel default_level () const noexcept {
        return _default.load ();
    }

    /**
     * @brief Last error text from the sink (thread-safe).
     * @note Meaningful after @ref Status::IoError.
     */
    std::string last_error () const;

    /**
     * @brief Flush the underlying sink, if supported.
     */
    void flush () const noexcept;

    private:
    std::unique_ptr<ILogSink> _sink; ///< Owned sink.
    std::atomic<LogLevel> _default;  ///< Current threshold.
    mutable std::mutex _mu;          ///< Protects @ref _last_err.
    std::string _last_err;           ///< Last sink error message.
};

/**
 * @brief Create a file sink for @p path.
 * @return Owned sink or nullptr on open error.
 */
std::unique_ptr<ILogSink> make_file_sink (const std::string& path) noexcept;

/**
 * @brief Create a TCP socket sink for @p host:@p port.
 * @return Owned sink or nullptr on connect error.
 */
std::unique_ptr<ILogSink> make_socket_sink (const std::string& host, std::uint16_t port) noexcept;
} // namespace logger
