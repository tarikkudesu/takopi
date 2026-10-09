#pragma once

#include <array>
#include <string>
#include <vector>

#include "game/Egg.hpp"
#include "game/GameEvent.hpp"
#include "game/Player.hpp"
#include "game/Team.hpp"
#include "game/Tile.hpp"

namespace zappy {

struct Incantation {
    int x = 0, y = 0, level = 0;
    float elapsed = 0;
};

// GameState owns the whole world as reported by the server (map, teams,
// players, eggs, incantations) plus the light animation/interpolation state
// needed to render it smoothly. It has no dependency on raylib or any other
// rendering library: MessageHandler feeds it, Renderer reads it.
class GameState {
public:
    void reset();

    // --- world -----------------------------------------------------------
    void setMapSize(int width, int height);
    void setTileContent(int x, int y, const std::array<int, kResourceCount>& res);
    void setTimeUnit(int freq) { timeUnit_ = freq; }

    int width() const { return width_; }
    int height() const { return height_; }
    bool hasMap() const { return width_ > 0 && height_ > 0; }
    const Tile* tileAt(int x, int y) const;
    const std::vector<Tile>& tiles() const { return tiles_; }
    int timeUnit() const { return timeUnit_; }

    // --- teams -------------------------------------------------------------
    int teamIndex(const std::string& name, bool createIfMissing);
    const std::vector<Team>& teams() const { return teams_; }

    // --- players -------------------------------------------------------------
    Player& playerJoin(int id, int x, int y, int orientation, int level, int team);
    void playerMove(int id, int x, int y, int orientation);
    void playerSetLevel(int id, int level);
    void playerSetInventory(int id, int x, int y, const std::array<int, kResourceCount>& inv);
    void playerExpel(int id);
    void playerBroadcast(int id, const std::string& text);
    void playerFork(int id);
    void playerTakeOrDrop(int id, int resource, bool take);
    void playerDie(int id);
    Player* findPlayer(int id);
    const Player* findPlayer(int id) const;
    const std::vector<Player>& players() const { return players_; }

    // --- eggs -------------------------------------------------------------
    void eggLaid(int eggId, int parentId, int x, int y);
    void eggHatchStart(int eggId);   // "eht": egg is ready to pop
    void eggConnected(int eggId);    // "ebo": a client connected through it
    void eggDied(int eggId);         // "edi": egg rotted
    const std::vector<Egg>& eggs() const { return eggs_; }

    // --- incantations -------------------------------------------------------
    void incantationStart(int x, int y, int level, const std::vector<int>& participantIds);
    void incantationEnd(int x, int y, bool success);
    const std::vector<Incantation>& incantations() const { return incantations_; }

    // --- game over ----------------------------------------------------------
    void setGameOver(const std::string& winnerTeam);
    bool isGameOver() const { return gameOver_; }
    const std::string& winnerName() const { return winnerName_; }
    int winnerTeamIndex() const { return winnerTeamIndex_; }

    void reportServerMessage(const std::string& text);
    void reportProtocolError(const std::string& text);

    // --- simulation ----------------------------------------------------------
    // Advances animation/interpolation state, ages eggs and incantations,
    // and removes entities whose death/end-of-life animation has finished.
    void update(float dt);

    // --- events ---------------------------------------------------------------
    std::vector<GameEvent> consumeEvents();

private:
    int width_ = 0, height_ = 0;
    std::vector<Tile> tiles_;
    std::vector<Team> teams_;
    std::vector<Player> players_;
    std::vector<Egg> eggs_;
    std::vector<Incantation> incantations_;
    int timeUnit_ = 100;
    bool gameOver_ = false;
    std::string winnerName_;
    int winnerTeamIndex_ = -1;
    std::vector<GameEvent> events_;

    Egg* findEgg(int id);
    void pushEvent(GameEvent ev) { events_.push_back(std::move(ev)); }
    void recomputeTeamStats();
    void moveTowards(Player& p, int x, int y);
};

} // namespace zappy
