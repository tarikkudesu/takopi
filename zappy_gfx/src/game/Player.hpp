#pragma once

#include <array>

#include "game/Constants.hpp"

namespace zappy {

// A player contains protocol state plus the interpolation state used while
// moving between map tiles.
struct Player {
    int id = -1;
    int team = -1;
    int x = 0, y = 0;
    int orientation = 1;   // 1 = North, 2 = East, 3 = South, 4 = West
    int level = 1;
    std::array<int, kResourceCount> inventory{};
    bool hasInventory = false;

    // Positions are in tile units and may temporarily run outside the map
    // while crossing a wrap-around edge.
    float fx = 0, fz = 0;
    float gx = 0, gz = 0;
    float ox = 0, oz = 0;      // offset when several players share a tile
    float dyingT = 0;          // removal grace period after a death event
    bool incanting = false;

    float scale() const { return 0.95f + 0.055f * level; }
};

} // namespace zappy
