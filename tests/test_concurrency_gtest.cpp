#include "logger/file_sink.hpp"
#include "logger/logger.hpp"
#include <atomic>
#include <filesystem>
#include <fstream>
#include <gtest/gtest.h>
#include <string>
#include <thread>
#include <vector>

using namespace logger;
namespace fs = std::filesystem;

TEST (Logger_Concurrency, MultipleThreadsProduceCompleteLines) {
    fs::path tmp = fs::temp_directory_path () / "logger_concurrency_mt.log";
    std::error_code ec;
    fs::remove (tmp, ec);

    auto sink = make_file_sink (tmp.string ());
    Logger L (std::move (sink), LogLevel::Info);

    constexpr int threads    = 4;
    constexpr int per_thread = 250;
    std::vector<std::thread> th;
    th.reserve (threads);
    std::atomic<int> started{ 0 };

    for (int t = 0; t < threads; ++t) {
        th.emplace_back ([&, t] {
            started.fetch_add (1);
            for (int i = 0; i < per_thread; ++i) {
                L.log (LogLevel::Info, "msg " + std::to_string (t) + "-" + std::to_string (i));
            }
        });
    }
    for (auto& x : th)
        x.join ();
    L.flush ();

    std::ifstream ifs (tmp);
    size_t lines = 0;
    std::string s;
    while (std::getline (ifs, s)) {
        if (s.find_first_not_of (" \t\r\n") != std::string::npos)
            ++lines;
    }

    EXPECT_EQ (lines, static_cast<size_t> (threads * per_thread));
}
