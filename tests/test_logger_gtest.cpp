#include "logger/file_sink.hpp"
#include "logger/logger.hpp"
#include <filesystem>
#include <fstream>
#include <gtest/gtest.h>
#include <regex>
#include <string>

using namespace logger;
namespace fs = std::filesystem;

static size_t count_lines (const fs::path& p) {
    std::ifstream ifs (p);
    size_t c = 0;
    std::string s;
    while (std::getline (ifs, s))
        if (!s.empty ())
            ++c;
    return c;
}

TEST (Logger, DefaultLevelFiltering) {
    const fs::path tmp = fs::temp_directory_path () / "logger_default_level.log";
    std::error_code ec;
    fs::remove (tmp, ec);

    auto sink = make_file_sink (tmp.string ());
    Logger L (std::move (sink), LogLevel::Warning);

    EXPECT_EQ (L.log (LogLevel::Info, "info filtered"), Status::Filtered);
    EXPECT_EQ (L.log (LogLevel::Warning, "warn kept"), Status::Ok);
    EXPECT_EQ (L.log (LogLevel::Error, "error kept"), Status::Ok);

    L.set_default_level (LogLevel::Info);
    EXPECT_EQ (L.default_level (), LogLevel::Info);
    EXPECT_EQ (L.log ("implicit default info"), Status::Ok);
    L.flush ();

    EXPECT_EQ (count_lines (tmp), 3u);
}

TEST (Logger, LastErrorPopulatedOnSinkFailure) {
    const fs::path bad_dir  = fs::temp_directory_path () / "nonexistent_dir_for_logger_tests2";
    const fs::path bad_path = bad_dir / "x.log";
    std::error_code ec;
    fs::remove_all (bad_dir, ec);

    auto sink = make_file_sink (bad_path.string ());
    Logger L (std::move (sink), LogLevel::Info);

    const auto st = L.log (LogLevel::Info, "will fail");
    EXPECT_EQ (st, Status::IoError);
    const auto msg = L.last_error ();
    EXPECT_FALSE (msg.empty ());
}
