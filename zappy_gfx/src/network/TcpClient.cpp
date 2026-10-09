#include "network/TcpClient.hpp"

#include <csignal>
#include <cstring>

#include <netdb.h>
#include <sys/socket.h>
#include <unistd.h>

namespace zappy {

TcpClient::~TcpClient() { disconnect(); }

bool TcpClient::connect(const std::string& host, int port) {
    disconnect();

    struct addrinfo hints{};
    hints.ai_family = AF_UNSPEC;
    hints.ai_socktype = SOCK_STREAM;

    struct addrinfo* res = nullptr;
    if (getaddrinfo(host.c_str(), std::to_string(port).c_str(), &hints, &res) != 0) 
    {
        lastError_ = "Cannot resolve host '" + host + "'";
        return false;
    }
    int fd = -1;
    for (struct addrinfo* rp = res; rp; rp = rp->ai_next) 
    {
        fd = socket(rp->ai_family, rp->ai_socktype, rp->ai_protocol);
        if (fd < 0) 
            continue;
        if (::connect(fd, rp->ai_addr, rp->ai_addrlen) == 0) 
            break;
        ::close(fd);
        fd = -1;
    }
    freeaddrinfo(res);
    if (fd < 0) 
    {
        lastError_ = "Cannot connect to " + host + ":" + std::to_string(port);
        return false;
    }
    std::signal(SIGPIPE, SIG_IGN);
    fd_ = fd;
    outBuffer_.clear();
    lastError_.clear();
    return true;
}

void TcpClient::disconnect() {
    if (fd_ >= 0) 
        ::close(fd_);
    fd_ = -1;
    outBuffer_.clear();
}

void TcpClient::send(const std::string& line) {
    if (fd_ < 0) return;
    outBuffer_ += line;
    flush();
}

void TcpClient::flush() {
    if (fd_ < 0 || outBuffer_.empty()) return;

    fd_set wfds;
    FD_ZERO(&wfds);
    FD_SET(fd_, &wfds);
    struct timeval tv{0, 0};
    if (select(fd_ + 1, nullptr, &wfds, nullptr, &tv) <= 0) 
        return ;

    ssize_t n = ::send(fd_, outBuffer_.data(), outBuffer_.size(), 0);
    if (n > 0) 
    {
        outBuffer_.erase(0, static_cast<size_t>(n));
    } 
    else if (n < 0 && errno != EINTR && errno != EAGAIN && errno != EWOULDBLOCK) 
    {
        lastError_ = "Connection lost (send failed)";
        disconnect();
    }
}

std::string TcpClient::poll() 
{
    std::string received;
    if (fd_ < 0) 
        return received;

    for (int guard = 0; guard < 64; guard++) {
        fd_set rfds;
        FD_ZERO(&rfds);
        FD_SET(fd_, &rfds);
        struct timeval tv{0, 0};
        int ready = select(fd_ + 1, &rfds, nullptr, nullptr, &tv);
        if (ready <= 0) break;   // nothing more to read right now

        char buf[65536];
        ssize_t n = recv(fd_, buf, sizeof buf, 0);
        if (n > 0) {
            received.append(buf, static_cast<size_t>(n));
        } else if (n == 0) {
            lastError_ = "Server closed the connection";
            disconnect();
            break;
        } else if (errno == EINTR) {
            continue;
        } else {
            lastError_ = "Connection lost (recv failed)";
            disconnect();
            break;
        }
    }

    flush();
    return received;
}

} // namespace zappy
