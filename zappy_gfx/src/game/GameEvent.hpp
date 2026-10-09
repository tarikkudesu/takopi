#pragma once

#include <string>

namespace zappy {

// GameState emits one of these each time something visually or audibly
// interesting happens, so the gfx layer can react (spawn particles, log a
// line, play a sound) without GameState knowing anything about rendering.
enum class GameEventKind {
    PlayerJoined,
    PlayerExpelled,
    PlayerBroadcast,
    IncantationStarted,
    IncantationEnded,
    PlayerFork,
    ResourceTaken,
    ResourceDropped,
    PlayerDied,
    EggLaid,
    EggHatched,
    EggEnded,
    GameOver,
    ServerMessage,
    ProtocolError,
};

struct GameEvent {
    GameEventKind kind;
    int id1 = -1;      // primary entity id (player or egg)
    int id2 = -1;      // secondary id, e.g. an egg's parent player
    int x = 0, y = 0;  // tile coordinates, when relevant
    int value = 0;     // level / incantation result / resource index / egg-end kind
    int teamIndex = -1;
    std::string text;  // broadcast / server message contents
};

} // namespace zappy
