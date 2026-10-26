#pragma once

#include <array>

#include "game/Constants.hpp"

namespace zappy 
{
    struct Tile {
        std::array<int, kResourceCount> resources{};
    };

}
