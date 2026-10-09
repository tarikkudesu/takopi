#pragma once

namespace zappy {

enum class EggState {
    Alive,       // laid, not yet hatched or resolved
    HatchedOut,  // a client connected through it (ebo)
    Rotten,      // died of hunger before anyone connected (edi)
};

struct Egg {
    int id = -1;
    int parent = -1;
    int team = -1;
    int x = 0, y = 0;
    bool hatched = false;   // "eht" was received: it is ready to pop
    float age = 0;
    float dyingT = 0;       // >0 while playing its end-of-life animation
    EggState state = EggState::Alive;
};

} // namespace zappy
