#pragma once

#include <string>
#include <vector>

#include "protocol/Message.hpp"

namespace zappy {
class Parser {
public:
    std::vector<Message> feed(const std::string& bytes);

private:
    std::string buffer_;

    static Message parseLine(std::string line);
};

} // namespace zappy
