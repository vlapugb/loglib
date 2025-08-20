#include "logger/socket_sink.hpp"
#include "logger/log_level.hpp"
#include "logger/utils.hpp"

#include <cstring>
#include <iostream>
#include <string>
#include <utility>

#include <netdb.h>
#include <netinet/tcp.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <unistd.h>

namespace logger {
SocketSink::SocketSink (std::string host, const std::uint16_t port) noexcept : _host (std::move (host)), _port (port) {
    std::string err;
    connect_socket (err);
}

SocketSink::~SocketSink () {
    close_socket ();
}

bool SocketSink::connect_socket (std::string& err) noexcept {
    if (_fd != -1)
        return true;
    addrinfo hints{};
    hints.ai_family   = AF_UNSPEC;
    hints.ai_socktype = SOCK_STREAM;

    addrinfo* res              = nullptr;
    const std::string port_str = std::to_string (_port);
    if (const int rc = getaddrinfo (_host.c_str (), port_str.c_str (), &hints, &res); rc != 0) {
        err = std::string ("get addr info: ") + gai_strerror (rc);
        return false;
    }

    for (const addrinfo* p = res; p != nullptr; p = p->ai_next) {
        int fd = socket (p->ai_family, p->ai_socktype, p->ai_protocol);
        if (fd == -1)
            continue;
        int one = 1;
        setsockopt (fd, IPPROTO_TCP, TCP_NODELAY, &one, sizeof (one));
        if (connect (fd, p->ai_addr, p->ai_addrlen) == 0) {
            _fd = fd;
            break;
        }
        close (fd);
        // fd = -1;
    }
    freeaddrinfo (res);
    if (_fd == -1) {
        err = "SocketSink: connect failed";
        return false;
    }
    return true;
}

void SocketSink::close_socket () noexcept {
    if (_fd != -1) {
        close (_fd);
        _fd = -1;
    }
}

bool SocketSink::write (const LogEntry& e, std::string& err) noexcept {
    std::lock_guard lk (_mu);
    if (_fd == -1) {
        if (!connect_socket (err))
            return false;
    }
    std::string line = std::to_string (e.epoch_ms);
    line += "|";
    line += std::string (to_string (e.level));
    line += "|";
    line += e.message;
    line += "\n";
    const char* data = line.c_str ();
    size_t left      = line.size ();
    while (left > 0) {
        ssize_t n = send (_fd, data, left, MSG_NOSIGNAL);
        if (n <= 0) {
            close_socket ();
            if (!connect_socket (err)) {
                err = "SocketSink: send failed and reconnect failed";
                return false;
            }
            n = send (_fd, data, left, MSG_NOSIGNAL);
            if (n <= 0) {
                err = "SocketSink: send failed after reconnect";
                return false;
            }
        }
        data += n;
        left -= static_cast<size_t> (n);
    }
    return true;
}
} // namespace logger
