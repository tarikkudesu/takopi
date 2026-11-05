#pragma once

namespace zappy 
{

    enum class EggState 
    {
        Alive,      
        HatchedOut,
        Rotten,
    };

    struct Egg {
        int id = -1;
        int parent = -1;
        int team = -1;
        int x = 0, y = 0;
        bool hatched = false;
        float age = 0;
        float dyingT = 0;
        EggState state = EggState::Alive;
    };

}
