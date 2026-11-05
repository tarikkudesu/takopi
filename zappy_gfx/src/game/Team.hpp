#pragma once

#include <string>

namespace zappy 
{
    struct Team 
    {
        std::string name;
        int colorIndex = 0;
        int count = 0;
        int maxLevel = 0;
        int level8Count = 0;
        int eggCount = 0;
    };

}
