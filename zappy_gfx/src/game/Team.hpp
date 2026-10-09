#pragma once

#include <string>

namespace zappy {

// A team as announced by "tna". Stats (count/maxLevel/level8Count/eggCount)
// are recomputed every tick by GameState::update, not maintained
// incrementally, so they can never drift out of sync.
//
// colorIndex is simply the team's creation order; the gfx layer owns the
// actual color palette and maps this index to a Color, so this struct has
// no dependency on raylib.
struct Team {
    std::string name;
    int colorIndex = 0;
    int count = 0;
    int maxLevel = 0;
    int level8Count = 0;
    int eggCount = 0;
};

} // namespace zappy
