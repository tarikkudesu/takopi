#pragma once

#include <array>

#include "game/Constants.hpp"

namespace zappy {

// One square of the map: how many of each resource kind lie on it.
// Index order matches RFC 4242: food, linemate, deraumere, sibur,
// mendiane, phiras, thystame.
struct Tile {
    std::array<int, kResourceCount> resources{};
};

} // namespace zappy
