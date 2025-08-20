#include "logger/stats.hpp"
#include "logger/log_level.hpp"
#include "logger/utils.hpp"

#include <atomic>
#include <cctype>
#include <chrono>
#include <csignal>
#include <cstdio>
#include <cstring>
#include <iostream>
#include <optional>
#include <string>
#include <thread>
#include <vector>

#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <unistd.h>
#include <fcntl.h>
#include <cerrno>

using namespace logger;

namespace stat_func {
    std::atomic<bool> g_stop{false};
    void on_sigint(int) { g_stop.store(true); }

    struct Options {
        std::uint16_t port = 5555;
        std::size_t trigger_n = 100;
        std::size_t timeout_s = 10;
    };

    void usage() {
        std::cerr
                << "Usage:\n"
                << "  stats_collector --port <p> [--n <N>] [--timeout <sec>]\n\n"
                << "Protocol: epoch_ms|LEVEL|message\\n where LEVEL in {INFO,WARN,ERROR}\n";
    }

    std::optional<Options> parse_args(int argc, char **argv) {
        Options o;
        for (int i = 1; i < argc; i++) {
            if (std::string a = argv[i]; a == "--help" || a == "-h") {
                usage();
                return std::nullopt;
            } else if (a == "--port" && i + 1 <
                       argc) { o.port = static_cast<std::uint16_t>(std::stoi(argv[++i])); } else if (
                a == "--n" && i + 1 < argc) { o.trigger_n = static_cast<std::size_t>(std::stoul(argv[++i])); } else if (
                a == "--timeout" && i + 1 <
                argc) { o.timeout_s = static_cast<std::size_t>(std::stoul(argv[++i])); } else {
                std::cerr << "Unknown arg: " << a << "\n";
                usage();
                return std::nullopt;
            }
        }
        if (o.port == 0) {
            std::cerr << "Port must be 1..65535\n";
            return std::nullopt;
        }
        if (o.trigger_n == 0 && o.timeout_s == 0) {
            std::cerr << "Either --n or --timeout must be non-zero\n";
            return std::nullopt;
        }
        return o;
    }

    bool parse_line(const std::string &line, std::uint64_t &epoch, LogLevel &lvl, std::string &msg) {
        const auto p1 = line.find('|');
        if (p1 == std::string::npos) return false;
        const auto p2 = line.find('|', p1 + 1);
        if (p2 == std::string::npos) return false;
        epoch = 0;
        for (size_t i = 0; i < p1; i++) {
            if (const auto c = static_cast<unsigned char>(line[i]); !std::isdigit(c)) return false;
            epoch = epoch * 10 + (line[i] - '0');
        }
        std::string lvl_s = line.substr(p1 + 1, p2 - (p1 + 1));
        for (auto &c: lvl_s) c = static_cast<char>(std::toupper(static_cast<unsigned char>(c)));
        if (lvl_s == "INFO") lvl = LogLevel::Info;
        else if (lvl_s == "WARN") lvl = LogLevel::Warning;
        else if (lvl_s == "ERROR") lvl = LogLevel::Error;
        else return false;
        msg = line.substr(p2 + 1);
        if (!msg.empty() && msg.back() == '\r') msg.pop_back();
        return true;
    }

    int make_server(std::uint16_t port) {
        const int fd = ::socket(AF_INET, SOCK_STREAM, 0);
        if (fd < 0) return -1;
        constexpr int one = 1;
        setsockopt(fd, SOL_SOCKET, SO_REUSEADDR, &one, sizeof(one));
        sockaddr_in addr{};
        addr.sin_family = AF_INET;
        addr.sin_addr.s_addr = htonl(INADDR_LOOPBACK);
        addr.sin_port = htons(port);
        if (bind(fd, reinterpret_cast<sockaddr *>(&addr), sizeof(addr)) < 0) {
            ::close(fd);
            return -1;
        }
        if (listen(fd, 64) < 0) {
            ::close(fd);
            return -1;
        }
        const int flags = fcntl(fd, F_GETFL, 0);
        fcntl(fd, F_SETFL, flags | O_NONBLOCK);
        return fd;
    }

    void client_loop(const int cfd, StatsCollector *stats, std::atomic<std::size_t> *since_last) {
        std::string buf;
        buf.reserve(4096);
        char tmp[4096];
        while (!g_stop.load()) {
            ssize_t n = ::recv(cfd, tmp, sizeof(tmp), 0);
            if (n == 0) break;
            if (n < 0) {
                if (errno == EINTR) continue;
                if (errno == EAGAIN || errno == EWOULDBLOCK) {
                    std::this_thread::sleep_for(std::chrono::milliseconds(10));
                    continue;
                }
                break;
            }
            buf.append(tmp, tmp + n);
            size_t pos = 0;
            while (true) {
                auto nl = buf.find('\n', pos);
                if (nl == std::string::npos) {
                    buf.erase(0, pos);
                    break;
                }
                std::string line = buf.substr(pos, nl - pos);
                pos = nl + 1;
                std::uint64_t epoch;
                std::string msg;
                if (LogLevel lvl; parse_line(line, epoch, lvl, msg)) {
                    stats->add(epoch, lvl, msg.size());
                    since_last->fetch_add(1, std::memory_order_relaxed);
                }
            }
        }
        ::close(cfd);
    }

    void print_snapshot(const StatsSnapshot &s) {
        std::cout << "=== stats ===\n"
                << "total: " << s.total << " (ERROR " << s.by_level[0]
                << ", WARN " << s.by_level[1] << ", INFO " << s.by_level[2] << ")\n"
                << "len: min " << s.min_len << ", max " << s.max_len << ", avg " << s.avg_len << "\n"
                << "last_hour: " << s.last_hour_total << " (ERROR " << s.last_hour_by_level[0]
                << ", WARN " << s.last_hour_by_level[1] << ", INFO " << s.last_hour_by_level[2]
                << "), avg_len " << s.last_hour_avg_len << "\n";
        std::cout.flush();
    }

    int main_impl(int argc, char **argv) {
        auto opt = parse_args(argc, argv);
        if (!opt) return 2;
        const auto [port, trigger_n, timeout_s] = *opt;

        std::signal(SIGINT, on_sigint);
        std::signal(SIGTERM, on_sigint);

        const int sfd = make_server(port);
        if (sfd < 0) {
            std::perror("bind/listen");
            return 1;
        }
        std::cout << "stats_collector listening on 127.0.0.1:" << port << "\n";
        StatsCollector stats;
        std::atomic<std::size_t> since_last{0};
        auto last_print = std::chrono::steady_clock::now();

        std::thread reporter([&]() {
            while (!g_stop.load()) {
                std::this_thread::sleep_for(std::chrono::milliseconds(200));
                const bool count_reached = (trigger_n > 0) && (since_last.load() >= trigger_n);
                const bool timeout_reached = (timeout_s > 0) && (
                                                 std::chrono::steady_clock::now() - last_print >= std::chrono::seconds(
                                                     timeout_s));
                if (count_reached || timeout_reached) {
                    if (since_last.load() > 0) {
                        auto snap = stats.snapshot(now_epoch_ms());
                        print_snapshot(snap);
                        since_last.store(0);
                    }
                    last_print = std::chrono::steady_clock::now();
                }
            }
        });

        std::vector<std::thread> workers;
        while (!g_stop.load()) {
            sockaddr_in cli{};
            socklen_t len = sizeof(cli);
            int cfd = ::accept(sfd, reinterpret_cast<sockaddr *>(&cli), &len);
            if (cfd < 0) {
                if (errno == EINTR) continue;
                if (errno == EAGAIN || errno == EWOULDBLOCK) {
                    std::this_thread::sleep_for(std::chrono::milliseconds(10));
                    continue;
                }
                break;
            }
            workers.emplace_back(client_loop, cfd, &stats, &since_last);
        }
        for (auto &t: workers) if (t.joinable()) t.join();
        g_stop.store(true);
        reporter.join();
        close(sfd);
        return 0;
    }
}

int main(int argc, char **argv) { return stat_func::main_impl(argc, argv); }
