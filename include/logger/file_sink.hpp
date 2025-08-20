#pragma once
/**
 * @file
 * @brief File-based log sink with basic thread-safety.
 */

#include "log_sink.hpp"
#include <fstream>
#include <mutex>

namespace logger {
/**
 * @brief Log sink that writes entries to a file.
 * @details Uses a mutex to serialize writes across threads.
 */
class FileSink final : public ILogSink {
    public:
    /**
     * @brief Construct and attempt to open @p path.
     * @param path Target file path.
     * @note Never throws (noexcept). Check @ref is_open().
     */
    explicit FileSink (const std::string& path) noexcept;

    /** @brief Close the stream if open. */
    ~FileSink () override;

    /**
     * @brief Write one log entry.
     * @param e Log entry to write.
     * @param err Error message on failure.
     * @return true on success, false on I/O error.
     * @note Thread-safe.
     */
    bool write (const LogEntry& e, std::string& err) noexcept override;

    /** @brief Flush the underlying stream. */
    void flush () noexcept override;

    /** @brief Whether the file stream is open. */
    bool is_open () const noexcept {
        return _ofs.is_open ();
    }

    private:
    /** @brief Owned output stream. */
    std::ofstream _ofs;
    /** @brief Guards stream operations. */
    std::mutex _mu;
};
} // namespace logger
