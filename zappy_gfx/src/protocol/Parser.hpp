#pragma once

#include <string>
#include <vector>

#include "protocol/Message.hpp"

namespace zappy {

// Reassembles TCP bytes, which may split or merge protocol lines in
// arbitrary ways, into complete Message objects. Keeps an internal buffer
// across calls so a command split across two poll()s is concatenated
// before being parsed, per RFC 4242.
class Parser {
public:
    std::vector<Message> feed(const std::string& bytes);

private:
    std::string buffer_;

    static Message parseLine(std::string line);
};

} // namespace zappy
