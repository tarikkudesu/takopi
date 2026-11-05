#pragma once

namespace zappy {

// World size, in render units, of one map tile. Used by the game layer for
// interpolation math and by the gfx layer for placing geometry.
const float kTileSize = 2.0f;

const int kResourceCount = 7;

const int kMaxTeams = 32;

// Animation durations, in seconds.
const float kDeathAnimTime = 1.2f;
const float kEggAnimTime = 0.6f;

// The server does not proactively announce resources respawning on a tile,
// so the client must periodically re-ask for the whole map with "mct".
const float kMapRefreshInterval = 3.0f;

}
