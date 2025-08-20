#include "logger/logger.hpp"
#include "logger/log_level.hpp"
#include "logger/file_sink.hpp"
#include "logger/socket_sink.hpp"
#include "logger/utils.hpp"

#include <cctype>
#include <cstring>
#include <iostream>
#include <memory>
#include <optional>
#include <string>
#include <string_view>
#include <vector>

using namespace logger;

namespace  log_app {
    struct Options {
        std::optional<std::string> file;
        std::optional<std::string> socket;
        LogLevel level = LogLevel::Info;
    };

    void usage() {
        std::cerr << "Usage:\n"
                << "  log_app --file <log.txt> --level <info|warn|error> [--socket <host:port>]\n\n"
                << "Interactive input format:\n"
                << "  [LEVEL] message\n"
                << "Examples:\n"
                << "  INFO System started\n"
                << "  WARN Low disk space\n"
                << "  Hello without level (uses default)\n"
                << "Commands:\n"
                << "  /level <info|warn|error>   - change default level\n"
                << "  /quit                      - exit\n";
    }

    std::optional<Options> parse_args(int argc, char **argv) {
        Options o;
        for (int i = 1; i < argc; i++) {
            if (std::string a = argv[i]; a == "--help" || a == "-h") {
                usage();
                return std::nullopt;
            } else if (a == "--file" && i + 1 < argc) { o.file = argv[++i]; } else if (
                a == "--socket" && i + 1 < argc) { o.socket = argv[++i]; } else if (a == "--level" && i + 1 < argc) {
                LogLevel tmp;
                if (!parse_level(argv[++i], tmp)) {
                    std::cerr << "Bad level\n";
                    return std::nullopt;
                }
                o.level = tmp;
            } else {
                std::cerr << "Unknown arg: " << a << "\n";
                usage();
                return std::nullopt;
            }
        }
        if (!o.file && !o.socket) {
            std::cerr << "Need at least --file or --socket\n";
            return std::nullopt;
        }
        return o;
    }

    class CompositeSink final : public ILogSink {
    public:
        explicit CompositeSink(std::vector<std::unique_ptr<ILogSink> > &&sinks) : _sinks(std::move(sinks)) {
        }

        bool write(const LogEntry &e, std::string &err) noexcept override {
            bool any_ok = false;
            std::string errs;
            for (const auto &s: _sinks) {
                if (std::string local; s->write(e, local)) {
                    any_ok = true;
                } else {
                    if (!errs.empty()) errs += "; ";
                    errs += local;
                }
            }
            if (!any_ok) err = errs.empty() ? "CompositeSink: all writes failed" : errs;
            return any_ok;
        }

        void flush() noexcept override {
            for (const auto &s: _sinks) s->flush();
        }

    private:
        std::vector<std::unique_ptr<ILogSink> > _sinks;
    };

    std::unique_ptr<ILogSink> make_composite_or_single(const Options &o) {
        std::vector<std::unique_ptr<ILogSink> > sinks;
        if (o.file) sinks.emplace_back(make_file_sink(*o.file));
        if (o.socket) {
            std::string host;
            std::uint16_t port = 0;
            if (!split_host_port(*o.socket, host, port)) {
                std::cerr << "Bad --socket value, expected host:port\n";
                return nullptr;
            }
            sinks.emplace_back(std::unique_ptr<ILogSink>(std::make_unique<SocketSink>(host, port)));
        }
        if (sinks.empty()) return nullptr;
        if (sinks.size() == 1) return std::move(sinks[0]);
        return std::make_unique<CompositeSink>(std::move(sinks));
    }

    void parse_leveled_line(const std::string_view line, const LogLevel def, LogLevel &out_lvl, std::string &out_msg) {
        if (line.size() >= 5) {
            std::string up;
            up.reserve(5);
            size_t i = 0;
            while (i < line.size() && !std::isspace(static_cast<unsigned char>(line[i])) && up.size() < 5) {
                up.push_back(static_cast<char>(std::toupper(static_cast<unsigned char>(line[i]))));
                ++i;
            }
            if (i < line.size() && std::isspace(static_cast<unsigned char>(line[i]))) {
                if (LogLevel tmp; parse_level(up, tmp)) {
                    while (i < line.size() && std::isspace(static_cast<unsigned char>(line[i]))) ++i;
                    out_lvl = tmp;
                    out_msg.assign(line.substr(i));
                }
            }
        }
        out_lvl = def;
        out_msg.assign(line);
    }
}

int main(int argc, char **argv) {
    const auto opt = log_app::parse_args(argc, argv);
    if (!opt) return 2;
    const auto& o = *opt;

    auto sink = make_composite_or_single(o);
    if (!sink) return 1;

    Logger L(std::move(sink), o.level);

    std::cerr << "Default level: " << to_string(L.default_level()) << "\n";
    std::cerr << "Enter lines (or /quit):\n";

    std::string line;
    while (std::getline(std::cin, line)) {
        if (line == "/quit") break;
        if (line.rfind("/level", 0) == 0) {
            if (const auto pos = line.find(' '); pos != std::string::npos) {
                std::string lvl = line.substr(pos + 1);
                if (LogLevel nv; parse_level(lvl, nv)) {
                    L.set_default_level(nv);
                    std::cerr << "Default level set to " << to_string(nv) << "\n";
                } else std::cerr << "Unknown level\n";
            } else {
                std::cerr << "Usage: /level <info|warn|error>\n";
            }
            continue;
        }
        LogLevel lvl;
        std::string msg;
        log_app::parse_leveled_line(line, L.default_level(), lvl, msg);
        if (const auto st = L.log(lvl, msg); st != Status::Ok) {
            std::cerr << "log failed: " << L.last_error() << "\n";
        }
    }
    L.flush();
    return 0;
}
