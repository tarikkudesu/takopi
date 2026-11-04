#pragma once

#include <string>

namespace zappy {

    class TcpClient 
    {
        public:
            TcpClient() = default;
            ~TcpClient();

            TcpClient(const TcpClient&) = delete;
            TcpClient& operator=(const TcpClient&) = delete;

            bool connect(const std::string& host, int port);
            void disconnect();
            bool isConnected() const { return fd_ >= 0; }
            void send(const std::string& line);
            std::string poll();
            const std::string& lastError() const 
            { 
                return lastError_; 
            }

        private:
            int fd_ = -1;
            std::string outBuffer_;
            std::string lastError_;

            void flush();
    };

}
