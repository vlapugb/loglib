#include "logger/utils.hpp"
#include <gtest/gtest.h>
#include <regex>
#include <string>

using namespace logger;

static bool iso_like_utc(const std::string &s) {
    static const std::regex re(R"(^\d{4}-\d{2}-\d{2}T\d{2}:\d{2}:\d{2}Z$)");
    return std::regex_match(s, re);
}

TEST(Utils, NowEpochMsMonotonicIncreasing) {
    const auto a = now_epoch_ms();
    const auto b = now_epoch_ms();
    EXPECT_LE(a, b);
}

TEST(Utils, Iso8601UTC_FormatOnly) {
    EXPECT_TRUE(iso_like_utc(iso8601_utc(0)));
    EXPECT_TRUE(iso_like_utc(iso8601_utc(1'600'000'000'000ULL)));
    const auto now = now_epoch_ms();
    EXPECT_TRUE(iso_like_utc(iso8601_utc(now)));
}

TEST(Utils, SplitHostPort) {
    std::string host;
    std::uint16_t port = 0;
    EXPECT_TRUE(split_host_port("localhost:1234", host, port));
    EXPECT_EQ(host, "localhost");
    EXPECT_EQ(port, 1234);

    EXPECT_TRUE(split_host_port("127.0.0.1:80", host, port));
    EXPECT_EQ(host, "127.0.0.1");
    EXPECT_EQ(port, 80);

    EXPECT_FALSE(split_host_port("no-colon", host, port));
    EXPECT_FALSE(split_host_port("host:", host, port));
    EXPECT_FALSE(split_host_port(":9090", host, port));
    EXPECT_FALSE(split_host_port("host:70000", host, port));
}
