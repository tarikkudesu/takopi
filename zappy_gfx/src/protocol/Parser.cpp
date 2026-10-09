#include "protocol/Parser.hpp"

#include <sstream>

namespace zappy {

std::vector<Message> Parser::feed(const std::string& bytes) {
    buffer_ += bytes;

    std::vector<Message> out;
    size_t start = 0;
    while (true) {
        size_t nl = buffer_.find('\n', start);
        if (nl == std::string::npos) break;
        std::string line = buffer_.substr(start, nl - start);
        start = nl + 1;
        if (!line.empty()) out.push_back(parseLine(std::move(line)));
    }
    buffer_.erase(0, start);

    // Guard against a misbehaving peer that never sends '\n'.
    if (buffer_.size() > (1u << 20)) buffer_.clear();

    return out;
}

Message Parser::parseLine(std::string line) {
    while (!line.empty() && (line.back() == '\r' || line.back() == ' ')) line.pop_back();

    Message msg;
    msg.raw = line;

    std::istringstream iss(line);
    std::string tok;
    bool first = true;
    while (iss >> tok) {
        if (first) { msg.command = tok; first = false; }
        else       msg.args.push_back(tok);
    }
    return msg;
}

} // namespace zappy
