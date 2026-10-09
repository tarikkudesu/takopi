#pragma once

#include <string>
#include <vector>

namespace zappy {

// One line of the RFC 4242 protocol, already split into a command and its
// whitespace-separated arguments. `raw` keeps the full trimmed line so
// MessageHandler can recover free-form text (broadcast messages, server
// messages) that tokenizing would otherwise mangle.
struct Message {
    std::string command;
    std::vector<std::string> args;
    std::string raw;
};

} // namespace zappy
