#pragma once
/**
 * @file
 * @brief Log sink interface.
 */

#include "log_entry.hpp"
#include <string>

namespace logger {
/**
 * @brief Abstract destination for log entries.
 */
class ILogSink {
    public:
    /** @brief Virtual destructor. */
    virtual ~ILogSink () = default;

    /**
     * @brief Write a single log entry.
     * @param e Entry to write.
     * @param err Error message on failure.
     * @return true on success, false on error.
     */
    virtual bool write (const LogEntry& e, std::string& err) noexcept = 0;

    /**
     * @brief Flush buffered data (optional).
     * @note Default impl does nothing.
     */
    virtual void flush () noexcept {
    }
};
} // namespace logger
