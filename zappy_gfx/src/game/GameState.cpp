#include "game/GameState.hpp"

#include <algorithm>
#include <cmath>

namespace zappy {

namespace {

float wrapf(float v, float m) {
    if (m <= 0) return v;
    float r = std::fmod(v, m);
    return r < 0 ? r + m : r;
}

float angleDiff(float a, float b) {
    float d = std::fmod(b - a, 360.0f);
    if (d > 180.0f) d -= 360.0f;
    if (d < -180.0f) d += 360.0f;
    return d;
}

float orientationYaw(int o) { return (o >= 1 && o <= 4) ? (o - 1) * 90.0f : 0.0f; }

constexpr float kPi = 3.14159265358979323846f;

} // namespace

void GameState::reset() {
    width_ = height_ = 0;
    tiles_.clear();
    teams_.clear();
    players_.clear();
    eggs_.clear();
    incantations_.clear();
    timeUnit_ = 100;
    gameOver_ = false;
    winnerName_.clear();
    winnerTeamIndex_ = -1;
    events_.clear();
}

void GameState::setMapSize(int width, int height) {
    if (width <= 0 || height <= 0) return;
    width_ = width;
    height_ = height;
    tiles_.assign(static_cast<size_t>(width) * static_cast<size_t>(height), Tile{});
}

void GameState::setTileContent(int x, int y, const std::array<int, kResourceCount>& res) {
    if (!hasMap() || x < 0 || y < 0 || x >= width_ || y >= height_) return;
    tiles_[static_cast<size_t>(y) * width_ + x].resources = res;
}

const Tile* GameState::tileAt(int x, int y) const {
    if (!hasMap() || x < 0 || y < 0 || x >= width_ || y >= height_) return nullptr;
    return &tiles_[static_cast<size_t>(y) * width_ + x];
}

int GameState::teamIndex(const std::string& name, bool createIfMissing) {
    for (size_t i = 0; i < teams_.size(); i++)
        if (teams_[i].name == name) return static_cast<int>(i);
    if (!createIfMissing || teams_.size() >= static_cast<size_t>(kMaxTeams)) return -1;
    Team t;
    t.name = name;
    t.colorIndex = static_cast<int>(teams_.size());
    teams_.push_back(std::move(t));
    return static_cast<int>(teams_.size()) - 1;
}

Player* GameState::findPlayer(int id) {
    for (auto& p : players_) if (p.id == id) return &p;
    return nullptr;
}

const Player* GameState::findPlayer(int id) const {
    for (const auto& p : players_) if (p.id == id) return &p;
    return nullptr;
}

Player& GameState::playerJoin(int id, int x, int y, int orientation, int level, int team) {
    Player* existing = findPlayer(id);
    if (!existing) {
        players_.push_back(Player{});
        existing = &players_.back();
    }
    Player& p = *existing;
    p.id = id;
    p.team = team;
    p.x = x;
    p.y = y;
    p.orientation = orientation;
    p.level = level;
    p.fx = p.gx = static_cast<float>(x);
    p.fz = p.gz = static_cast<float>(y);
    p.yaw = p.goalYaw = orientationYaw(orientation);
    p.spawnT = 0;
    p.dyingT = 0;
    pushEvent({GameEventKind::PlayerJoined, id, -1, x, y, level, team, {}});
    return p;
}

void GameState::moveTowards(Player& p, int x, int y) {
    float cx = wrapf(p.fx, static_cast<float>(width_));
    float cz = wrapf(p.fz, static_cast<float>(height_));
    float dx = x - cx, dz = y - cz;
    if (dx > width_ / 2.0f) dx -= width_;
    if (dx < -width_ / 2.0f) dx += width_;
    if (dz > height_ / 2.0f) dz -= height_;
    if (dz < -height_ / 2.0f) dz += height_;
    p.gx = p.fx + dx;
    p.gz = p.fz + dz;
    p.x = x;
    p.y = y;
}

void GameState::playerMove(int id, int x, int y, int orientation) {
    Player* p = findPlayer(id);
    if (!p) return;
    if (x != p->x || y != p->y) moveTowards(*p, x, y);
    p->orientation = orientation;
    p->goalYaw = orientationYaw(orientation);
}

void GameState::playerSetLevel(int id, int level) {
    if (Player* p = findPlayer(id)) p->level = level;
}

void GameState::playerSetInventory(int id, int x, int y, const std::array<int, kResourceCount>& inv) {
    Player* p = findPlayer(id);
    if (!p) return;
    if (x != p->x || y != p->y) moveTowards(*p, x, y);
    p->inventory = inv;
    p->hasInventory = true;
}

void GameState::playerExpel(int id) {
    Player* p = findPlayer(id);
    if (!p) return;
    p->shakeT = 0.6f;
    pushEvent({GameEventKind::PlayerExpelled, id, -1, p->x, p->y, 0, p->team, {}});
}

void GameState::playerBroadcast(int id, const std::string& text) {
    Player* p = findPlayer(id);
    if (p) p->broadcastT = 1.0f;
    pushEvent({GameEventKind::PlayerBroadcast, id, -1, p ? p->x : 0, p ? p->y : 0, 0,
              p ? p->team : -1, text});
}

void GameState::playerFork(int id) {
    Player* p = findPlayer(id);
    if (p) p->layT = 1.0f;
    pushEvent({GameEventKind::PlayerFork, id, -1, p ? p->x : 0, p ? p->y : 0, 0,
              p ? p->team : -1, {}});
}

void GameState::playerTakeOrDrop(int id, int resource, bool take) {
    Player* p = findPlayer(id);
    if (!p || resource < 0 || resource >= kResourceCount) return;
    pushEvent({take ? GameEventKind::ResourceTaken : GameEventKind::ResourceDropped,
              id, -1, p->x, p->y, resource, p->team, {}});
}

void GameState::playerDie(int id) {
    Player* p = findPlayer(id);
    if (!p || p->dyingT > 0) return;
    p->dyingT = kDeathAnimTime;
    pushEvent({GameEventKind::PlayerDied, id, -1, p->x, p->y, 0, p->team, {}});
}

Egg* GameState::findEgg(int id) {
    for (auto& e : eggs_) if (e.id == id) return &e;
    return nullptr;
}

void GameState::eggLaid(int eggId, int parentId, int x, int y) {
    Egg* existing = findEgg(eggId);
    if (!existing) {
        eggs_.push_back(Egg{});
        existing = &eggs_.back();
    }
    Egg& e = *existing;
    e.id = eggId;
    e.parent = parentId;
    const Player* parent = findPlayer(parentId);
    e.team = parent ? parent->team : -1;
    e.x = x;
    e.y = y;
    e.hatched = false;
    e.age = 0;
    e.dyingT = 0;
    e.state = EggState::Alive;
    pushEvent({GameEventKind::EggLaid, eggId, parentId, x, y, 0, e.team, {}});
}

void GameState::eggHatchStart(int eggId) {
    Egg* e = findEgg(eggId);
    if (!e) return;
    e->hatched = true;
    pushEvent({GameEventKind::EggHatched, eggId, -1, e->x, e->y, 0, e->team, {}});
}

void GameState::eggConnected(int eggId) {
    Egg* e = findEgg(eggId);
    if (!e) return;
    e->state = EggState::HatchedOut;
    e->dyingT = kEggAnimTime;
    pushEvent({GameEventKind::EggEnded, eggId, -1, e->x, e->y, 1, e->team, {}});
}

void GameState::eggDied(int eggId) {
    Egg* e = findEgg(eggId);
    if (!e) return;
    e->state = EggState::Rotten;
    e->dyingT = kEggAnimTime;
    pushEvent({GameEventKind::EggEnded, eggId, -1, e->x, e->y, 2, e->team, {}});
}

void GameState::incantationStart(int x, int y, int level, const std::vector<int>& participantIds) {
    for (int id : participantIds)
        if (Player* p = findPlayer(id)) p->incanting = true;

    Incantation* found = nullptr;
    for (auto& inc : incantations_)
        if (inc.x == x && inc.y == y) found = &inc;
    if (found) { found->level = level; found->elapsed = 0; }
    else       incantations_.push_back({x, y, level, 0});

    pushEvent({GameEventKind::IncantationStarted, -1, -1, x, y, level, -1, {}});
}

void GameState::incantationEnd(int x, int y, bool success) {
    incantations_.erase(
        std::remove_if(incantations_.begin(), incantations_.end(),
                       [&](const Incantation& inc) { return inc.x == x && inc.y == y; }),
        incantations_.end());
    for (auto& p : players_)
        if (p.x == x && p.y == y) p.incanting = false;
    pushEvent({GameEventKind::IncantationEnded, -1, -1, x, y, success ? 1 : 0, -1, {}});
}

void GameState::setGameOver(const std::string& winnerTeam) {
    gameOver_ = true;
    winnerName_ = winnerTeam;
    winnerTeamIndex_ = teamIndex(winnerTeam, false);
    pushEvent({GameEventKind::GameOver, -1, -1, 0, 0, 0, winnerTeamIndex_, winnerTeam});
}

void GameState::reportServerMessage(const std::string& text) {
    pushEvent({GameEventKind::ServerMessage, -1, -1, 0, 0, 0, -1, text});
}

void GameState::reportProtocolError(const std::string& text) {
    pushEvent({GameEventKind::ProtocolError, -1, -1, 0, 0, 0, -1, text});
}

void GameState::recomputeTeamStats() {
    for (auto& t : teams_) { t.count = 0; t.maxLevel = 0; t.level8Count = 0; t.eggCount = 0; }
    for (const auto& p : players_) {
        if (p.team < 0 || p.team >= static_cast<int>(teams_.size()) || p.dyingT > 0) continue;
        Team& t = teams_[p.team];
        t.count++;
        t.maxLevel = std::max(t.maxLevel, p.level);
        if (p.level >= 8) t.level8Count++;
    }
    for (const auto& e : eggs_)
        if (e.team >= 0 && e.team < static_cast<int>(teams_.size()) && e.state == EggState::Alive)
            teams_[e.team].eggCount++;
}

void GameState::update(float dt) {
    // Players: glide toward their goal tile and settle their facing angle.
    const float speed = std::max(static_cast<float>(timeUnit_) / 7.0f, 1.0f) * 1.2f;
    for (size_t i = 0; i < players_.size(); ) {
        Player& p = players_[i];
        if (p.spawnT < 1) p.spawnT = std::min(1.0f, p.spawnT + dt * 3.0f);

        float dx = p.gx - p.fx, dz = p.gz - p.fz;
        float d = std::sqrt(dx * dx + dz * dz);
        if (d > 0.001f) {
            float step = std::max(speed, d * 6.0f) * dt;
            if (step >= d) { p.fx = p.gx; p.fz = p.gz; }
            else { p.fx += dx / d * step; p.fz += dz / d * step; }
            p.moving = true;
            p.animPhase += dt * 11.0f;
        } else {
            p.moving = false;
        }

        if (width_ > 0 && height_ > 0) {
            while (p.fx >= width_)  { p.fx -= width_;  p.gx -= width_; }
            while (p.fx < 0)        { p.fx += width_;  p.gx += width_; }
            while (p.fz >= height_) { p.fz -= height_; p.gz -= height_; }
            while (p.fz < 0)        { p.fz += height_; p.gz += height_; }
        }

        p.yaw += angleDiff(p.yaw, p.goalYaw) * std::min(1.0f, dt * 14.0f);
        p.shakeT = std::max(0.0f, p.shakeT - dt);
        p.layT = std::max(0.0f, p.layT - dt);
        p.broadcastT = std::max(0.0f, p.broadcastT - dt);

        if (p.dyingT > 0) {
            p.dyingT -= dt;
            if (p.dyingT <= 0) {
                players_[i] = players_.back();
                players_.pop_back();
                continue;
            }
        }
        i++;
    }

    // Spread players sharing a tile into small slots around its center.
    for (size_t i = 0; i < players_.size(); i++) {
        Player& p = players_[i];
        int cnt = 0, slot = 0;
        for (size_t j = 0; j < players_.size(); j++)
            if (players_[j].x == p.x && players_[j].y == p.y) {
                cnt++;
                if (j < i) slot++;
            }
        float tx = 0, tz = 0;
        if (cnt > 1) {
            float a = 2.0f * kPi * slot / cnt + 0.6f;
            float r = cnt > 5 ? 0.72f : 0.5f;
            tx = std::cos(a) * r;
            tz = std::sin(a) * r;
        }
        float k = std::min(1.0f, dt * 8.0f);
        p.ox += (tx - p.ox) * k;
        p.oz += (tz - p.oz) * k;
    }

    // Eggs: age, and remove once their end-of-life animation is done.
    for (size_t i = 0; i < eggs_.size(); ) {
        Egg& e = eggs_[i];
        e.age += dt;
        if (e.state != EggState::Alive) {
            e.dyingT -= dt;
            if (e.dyingT <= 0) {
                eggs_[i] = eggs_.back();
                eggs_.pop_back();
                continue;
            }
        }
        i++;
    }

    for (auto& inc : incantations_) inc.elapsed += dt;

    recomputeTeamStats();
}

std::vector<GameEvent> GameState::consumeEvents() {
    std::vector<GameEvent> out;
    out.swap(events_);
    return out;
}

} // namespace zappy
