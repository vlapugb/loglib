#include "logger/file_sink.hpp"
#include "logger/log_level.hpp"
#include "logger/logger.hpp"
#include <filesystem>
#include <fstream>
#include <gtest/gtest.h>
#include <regex>
#include <string>

using namespace logger;
namespace fs = std::filesystem;

static std::string read_all (const fs::path& p) {
    std::ifstream ifs (p);
    std::ostringstream oss;
    oss << ifs.rdbuf ();
    return oss.str ();
}

TEST (FileSink, BasicWriteAndFormat) {
    fs::path tmp = fs::temp_directory_path () / "logger_file_sink_basic.log";
    std::error_code ec;
    fs::remove (tmp, ec);

    auto sink = make_file_sink (tmp.string ());
    Logger L (std::move (sink), LogLevel::Info);

    EXPECT_EQ (L.log (LogLevel::Error, "err one"), Status::Ok);
    EXPECT_EQ (L.log (LogLevel::Warning, "warn two"), Status::Ok);
    EXPECT_EQ (L.log (LogLevel::Info, "info three"), Status::Ok);
    L.flush ();

    std::string all = read_all (tmp);
    std::regex re (R"(^\d{4}-\d{2}-\d{2}T\d{2}:\d{2}:\d{2}Z (ERROR|WARN|INFO) .*$)");

    size_t lines = 0;
    std::istringstream iss (all);
    std::string line;
    while (std::getline (iss, line)) {
        if (line.empty ())
            continue;
        ++lines;
        EXPECT_TRUE (std::regex_match (line, re)) << "bad line: " << line;
    }
    EXPECT_EQ (lines, 3u);
}

TEST (FileSink, IoErrorIsReported) {
    const fs::path bad_dir  = fs::temp_directory_path () / "nonexistent_dir_for_logger_tests";
    const fs::path bad_path = bad_dir / "x.log";
    std::error_code ec;
    fs::remove_all (bad_dir, ec); // make sure it doesn't exist

    FileSink sink (bad_path.string ());
    LogEntry e;
    e.epoch_ms = 0;
    e.level    = LogLevel::Error;
    e.message  = "test";

    std::string err;
    const bool ok = sink.write (e, err);
    EXPECT_FALSE (ok);
    EXPECT_FALSE (err.empty ());
    EXPECT_NE (err.find ("FileSink"), std::string::npos);
}
