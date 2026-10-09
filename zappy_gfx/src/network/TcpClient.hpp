#pragma once

#include <string>

namespace zappy {

// A small blocking TCP client for the graphical monitor.
//
// The subject explicitly forbids non-blocking sockets (no
// fcntl(fd, F_SETFL, O_NONBLOCK)). This class never sets that flag: the
// socket stays blocking. Instead, poll()/flush() call select() with a
// zero timeout to check readiness *before* calling recv()/send(), so a
// read or write is only attempted when it is guaranteed not to block the
// render loop.
class TcpClient {
public:
    TcpClient() = default;
    ~TcpClient();

    TcpClient(const TcpClient&) = delete;
    TcpClient& operator=(const TcpClient&) = delete;

    bool connect(const std::string& host, int port);
    void disconnect();
    bool isConnected() const { return fd_ >= 0; }

    // Queues bytes to send and immediately attempts to flush them.
    void send(const std::string& line);

    // Drains whatever the peer has sent so far (may be empty) without ever
    // blocking, and opportunistically flushes any still-queued output.
    std::string poll();

    const std::string& lastError() const { return lastError_; }

private:
    int fd_ = -1;
    std::string outBuffer_;
    std::string lastError_;

    void flush();
};

} // namespace zappy
