#pragma once

#include <string>
#include <vector>

namespace zappy {

struct Message 
{
    std::string command;
    std::vector<std::string> args;
    std::string raw;
};

}
