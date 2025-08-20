#include "logger/log_level.hpp"
#include "logger/logger.hpp"
#include "logger/socket_sink.hpp"
#include <atomic>
#include <cstring>
#include <gtest/gtest.h>
#include <string>
#include <thread>

#include <fcntl.h>
#include <netinet/in.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <unistd.h>

using namespace logger;

static int make_listen_ipv4 (uint16_t& out_port) {
    const int fd = ::socket (AF_INET, SOCK_STREAM, 0);
    if (fd < 0)
        return -1;
    constexpr int one = 1;
    setsockopt (fd, SOL_SOCKET, SO_REUSEADDR, &one, sizeof (one));
    sockaddr_in addr{};
    addr.sin_family      = AF_INET;
    addr.sin_addr.s_addr = htonl (INADDR_LOOPBACK);
    addr.sin_port        = 0;
    if (bind (fd, reinterpret_cast<sockaddr*> (&addr), sizeof (addr)) < 0) {
        ::close (fd);
        return -1;
    }
    socklen_t len = sizeof (addr);
    if (getsockname (fd, reinterpret_cast<sockaddr*> (&addr), &len) == 0) {
        out_port = ntohs (addr.sin_port);
    } else {
        ::close (fd);
        return -1;
    }
    if (listen (fd, 1) < 0) {
        ::close (fd);
        return -1;
    }
    const int flags = fcntl (fd, F_GETFL, 0);
    fcntl (fd, F_SETFL, flags | O_NONBLOCK);
    return fd;
}

TEST (SocketSink, SendsEpochLevelMessageLines) {
    uint16_t port = 0;
    int lfd       = make_listen_ipv4 (port);
    ASSERT_GE (lfd, 0);

    std::atomic<bool> have_line{ false };
    std::string received;
    std::atomic<bool> server_done{ false };

    std::thread server ([&] () {
        int cli = -1;
        for (int i = 0; i < 300 && cli < 0; ++i) {
            cli = ::accept (lfd, nullptr, nullptr);
            if (cli < 0)
                std::this_thread::sleep_for (std::chrono::milliseconds (10));
        }
        ASSERT_GE (cli, 0) << "accept timeout";
        timeval tv{};
        tv.tv_sec  = 2;
        tv.tv_usec = 0;
        setsockopt (cli, SOL_SOCKET, SO_RCVTIMEO, &tv, sizeof (tv));
        char buf[4096];
        ssize_t n;
        while ((n = ::recv (cli, buf, sizeof (buf), 0)) > 0) {
            received.append (buf, buf + n);
            if (received.find ('\n') != std::string::npos)
                break;
        }
        close (cli);
        close (lfd);
        have_line.store (true);
        server_done.store (true);
    });

    Logger L (std::make_unique<SocketSink> ("127.0.0.1", port), LogLevel::Info);
    EXPECT_EQ (L.log (LogLevel::Warning, "hello world"), Status::Ok);
    L.flush ();

    for (int i = 0; i < 300 && !have_line.load (); ++i)
        std::this_thread::sleep_for (std::chrono::milliseconds (10));
    server.join ();
    ASSERT_TRUE (server_done.load ());

    auto pos = received.find ('\n');
    ASSERT_NE (pos, std::string::npos);
    auto line = received.substr (0, pos);
    auto p1   = line.find ('|');
    ASSERT_NE (p1, std::string::npos);
    auto p2 = line.find ('|', p1 + 1);
    ASSERT_NE (p2, std::string::npos);
    auto epoch_str = line.substr (0, p1);
    auto level_str = line.substr (p1 + 1, p2 - (p1 + 1));
    auto msg_str   = line.substr (p2 + 1);

    ASSERT_FALSE (epoch_str.empty ());
    for (char c : epoch_str)
        ASSERT_TRUE (isdigit (static_cast<unsigned char> (c)));
    EXPECT_EQ (level_str, "WARN");
    EXPECT_EQ (msg_str, "hello world");
}
