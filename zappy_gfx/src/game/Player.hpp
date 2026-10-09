#pragma once

#include <array>

#include "game/Constants.hpp"

namespace zappy {

// A player, both its protocol-reported state (x, y, orientation, level,
// inventory) and the extra animation state GameState::update uses to make
// it glide smoothly between tiles instead of teleporting.
struct Player {
    int id = -1;
    int team = -1;
    int x = 0, y = 0;
    int orientation = 1;   // 1 = North, 2 = East, 3 = South, 4 = West
    int level = 1;
    std::array<int, kResourceCount> inventory{};
    bool hasInventory = false;

    // Logical render position (fx,fz) glides toward the goal (gx,gz) each
    // frame; both are in tile units and may run outside [0,width) while
    // crossing the map's wrap-around edge, unwrapped lazily by the renderer.
    float fx = 0, fz = 0;
    float gx = 0, gz = 0;
    float ox = 0, oz = 0;      // small offset when several players share a tile
    float yaw = 0, goalYaw = 0;
    float spawnT = 0;          // 0..1 spawn-in animation
    float dyingT = 0;          // >0 while playing the death animation
    float animPhase = 0;       // walk-cycle phase
    float shakeT = 0;          // expulsion shake
    float layT = 0;            // fork/lay pulse
    float broadcastT = 0;      // talking pulse
    bool moving = false;
    bool incanting = false;

    float scale() const { return 0.95f + 0.055f * level; }
};

} // namespace zappy
