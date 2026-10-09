#pragma once

namespace zappy {

// World size, in render units, of one map tile. Used by the game layer for
// interpolation math and by the gfx layer for placing geometry.
constexpr float kTileSize = 2.0f;

// Number of resource kinds: food, linemate, deraumere, sibur, mendiane,
// phiras, thystame (RFC 4242).
constexpr int kResourceCount = 7;

constexpr int kMaxTeams = 32;

// Animation durations, in seconds.
constexpr float kDeathAnimTime = 1.2f;
constexpr float kEggAnimTime = 0.6f;

// The server does not proactively announce resources respawning on a tile,
// so the client must periodically re-ask for the whole map with "mct".
constexpr float kMapRefreshInterval = 3.0f;

} // namespace zappy
