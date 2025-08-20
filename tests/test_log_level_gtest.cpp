#include "logger/log_level.hpp"
#include <gtest/gtest.h>
#include <string>

using namespace logger;

TEST(LogLevel, ToStringAndParse) {
    EXPECT_EQ(std::string(to_string(LogLevel::Info)), "INFO");
    EXPECT_EQ(std::string(to_string(LogLevel::Warning)), "WARN");
    EXPECT_EQ(std::string(to_string(LogLevel::Error)), "ERROR");

    LogLevel out{};
    EXPECT_TRUE(parse_level("INFO", out));
    EXPECT_EQ(out, LogLevel::Info);
    EXPECT_TRUE(parse_level("info", out));
    EXPECT_EQ(out, LogLevel::Info);
    EXPECT_TRUE(parse_level("WARN", out));
    EXPECT_EQ(out, LogLevel::Warning);
    EXPECT_TRUE(parse_level("warn", out));
    EXPECT_EQ(out, LogLevel::Warning);
    EXPECT_TRUE(parse_level("warning", out));
    EXPECT_EQ(out, LogLevel::Warning);
    EXPECT_TRUE(parse_level("ERROR", out));
    EXPECT_EQ(out, LogLevel::Error);
    EXPECT_TRUE(parse_level("error", out));
    EXPECT_EQ(out, LogLevel::Error);
    EXPECT_TRUE(parse_level("err", out));
    EXPECT_EQ(out, LogLevel::Error);

    EXPECT_FALSE(parse_level("debug", out));
    EXPECT_FALSE(parse_level("", out));
}
