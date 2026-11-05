#pragma once

#include <array>

#include "game/Constants.hpp"

namespace zappy 
{
    struct Player 
    {
        int id = -1;
        int team = -1;
        int x = 0;
        int y = 0;
        int orientation = 1;   // 1 = North, 2 = East, 3 = South, 4 = West
        int level = 1;
        std::array<int, kResourceCount> inventory{};
        bool hasInventory = false;

        float fx = 0, fz = 0;
        float gx = 0, gz = 0;
        float ox = 0, oz = 0;
        float dyingT = 0;
        bool incanting = false;

        float scale() const { return 0.95f + 0.055f * level; }
    };

} 
