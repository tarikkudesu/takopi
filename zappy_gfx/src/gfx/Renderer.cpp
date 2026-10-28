#include "gfx/Renderer.hpp"

#include <algorithm>
#include <cmath>
#include <cstdarg>
#include <cstdio>

#include "raymath.h"
#include "rlgl.h"

namespace zappy {

namespace {

constexpr float kPi = 3.14159265358979323846f;

constexpr int kResColCount = kResourceCount;
const char* kResName[kResColCount] = {
    "food", "linemate", "deraumere", "sibur", "mendiane", "phiras", "thystame"
};
const Color kResColor[kResColCount] = {
    {225, 70, 60, 255}, {215, 215, 225, 255}, {70, 140, 255, 255}, {60, 220, 120, 255},
    {255, 215, 60, 255}, {175, 95, 245, 255}, {255, 90, 185, 255},
};
const char* kOrientName[5] = {"?", "North", "East", "South", "West"};

const Color kTeamPalette[] = {
    {235, 64, 52, 255}, {52, 152, 235, 255}, {80, 200, 90, 255}, {250, 190, 40, 255},
    {170, 90, 230, 255}, {255, 130, 40, 255}, {40, 210, 200, 255}, {240, 100, 180, 255},
    {160, 200, 50, 255}, {110, 110, 245, 255}, {200, 140, 90, 255}, {235, 235, 235, 255},
};
constexpr int kPaletteSize = static_cast<int>(sizeof(kTeamPalette) / sizeof(kTeamPalette[0]));

Color mixColor(Color a, Color b, float t) {
    return Color{
        static_cast<unsigned char>(a.r + (b.r - a.r) * t),
        static_cast<unsigned char>(a.g + (b.g - a.g) * t),
        static_cast<unsigned char>(a.b + (b.b - a.b) * t),
        static_cast<unsigned char>(a.a + (b.a - a.a) * t),
    };
}

std::string truncated(const std::string& s, size_t maxLen) {
    return s.size() <= maxLen ? s : s.substr(0, maxLen);
}

void drawText(int x, int y, int size, Color c, const char* fmt, ...) {
    char buf[256];
    va_list ap;
    va_start(ap, fmt);
    vsnprintf(buf, sizeof buf, fmt, ap);
    va_end(ap);
    DrawText(buf, x + 1, y + 1, size, Color{0, 0, 0, 170});
    DrawText(buf, x, y, size, c);
}

void drawTextCentered(int cx, int y, int size, Color c, const char* fmt, ...) {
    char buf[256];
    va_list ap;
    va_start(ap, fmt);
    vsnprintf(buf, sizeof buf, fmt, ap);
    va_end(ap);
    int w = MeasureText(buf, size);
    DrawText(buf, cx - w / 2 + 1, y + 1, size, Color{0, 0, 0, 170});
    DrawText(buf, cx - w / 2, y, size, c);
}

} // namespace

// ------------------------------------------------------------------ lifecycle

bool Renderer::init(int width, int height, const char* title) {
    SetConfigFlags(FLAG_MSAA_4X_HINT | FLAG_WINDOW_RESIZABLE | FLAG_VSYNC_HINT);
    InitWindow(width, height, title);
    if (!IsWindowReady()) return false;

    SetExitKey(KEY_NULL);
    SetTargetFPS(60);
    SetWindowMinSize(800, 500);

    resetCamera();
    loadSounds();
    return true;
}

void Renderer::shutdown() {
    unloadSounds();
    CloseWindow();
}

bool Renderer::shouldClose() const { return WindowShouldClose(); }

void Renderer::beginFrame() {
    BeginDrawing();
    ClearBackground(Color{84, 116, 160, 255});
}

void Renderer::endFrame() { EndDrawing(); }

// -------------------------------------------------------------------- helpers

Color Renderer::teamColor(int teamIndex) const {
    if (teamIndex < 0) return Color{170, 170, 170, 255};
    return kTeamPalette[teamIndex % kPaletteSize];
}

void Renderer::resetCamera() {
    float w = state_.width() > 0 ? static_cast<float>(state_.width()) : 10.0f;
    float h = state_.height() > 0 ? static_cast<float>(state_.height()) : 10.0f;
    cameraTarget_ = {(w - 1) * 0.5f * kTileSize, 0, (h - 1) * 0.5f * kTileSize};
    cameraYaw_ = 0;
    cameraPitch_ = 0.95f;
    cameraDist_ = cameraDistGoal_ = Clamp(std::max(w, h) * kTileSize * 1.05f, 14, 300);
    follow_ = false;
}

void Renderer::log(Color color, const std::string& text) {
    if (log_.size() >= 32) log_.erase(log_.begin());
    log_.push_back({text, color, GetTime()});
}

void Renderer::selectPlayer(int id) {
    selectedPlayer_ = id;
    selectedTile_ = false;
    selectionRefreshTimer_ = 0;
    net_.send("ppo #" + std::to_string(id) + "\n");
    net_.send("plv #" + std::to_string(id) + "\n");
    net_.send("pin #" + std::to_string(id) + "\n");
}

void Renderer::selectTile(int x, int y) {
    selectedPlayer_ = -1;
    selectedTile_ = true;
    selectedTileX_ = x;
    selectedTileY_ = y;
    selectionRefreshTimer_ = 0;
    follow_ = false;
    net_.send("bct " + std::to_string(x) + " " + std::to_string(y) + "\n");
}

void Renderer::clearSelection() {
    selectedPlayer_ = -1;
    selectedTile_ = false;
    follow_ = false;
}

void Renderer::changeFrequency(int direction) {
    if (!net_.isConnected() || state_.timeUnit() <= 0) return;
    int f = direction > 0 ? static_cast<int>(state_.timeUnit() * 1.25f) + 1
                          : static_cast<int>(state_.timeUnit() / 1.25f);
    f = static_cast<int>(Clamp(static_cast<float>(f), 1, 10000));
    net_.send("sst " + std::to_string(f) + "\n");
}

Sound Renderer::makeTone(float freqHz, float durationSec, float volume) {
    constexpr int sampleRate = 44100;
    int frames = static_cast<int>(durationSec * sampleRate);
    std::vector<short> samples(static_cast<size_t>(std::max(frames, 1)));
    for (int i = 0; i < frames; i++) {
        float t = static_cast<float>(i) / sampleRate;
        float envelope = std::sin(kPi * static_cast<float>(i) / frames);
        samples[static_cast<size_t>(i)] =
            static_cast<short>(std::sin(2 * kPi * freqHz * t) * envelope * volume * 32000);
    }
    Wave wave{};
    wave.frameCount = static_cast<unsigned int>(samples.size());
    wave.sampleRate = sampleRate;
    wave.sampleSize = 16;
    wave.channels = 1;
    wave.data = samples.data();
    return LoadSoundFromWave(wave);
}

void Renderer::loadSounds() {
    InitAudioDevice();
    audioReady_ = IsAudioDeviceReady();
    if (!audioReady_) return;   // headless/no sound card: everything below is skipped at call sites

    sndBroadcast_ = makeTone(880.0f, 0.18f, 0.5f);
}

void Renderer::unloadSounds() {
    if (!audioReady_) return;
    UnloadSound(sndBroadcast_);
    UnloadSound(sndBroadcast_);
    CloseAudioDevice();
    audioReady_ = false;
}

float Renderer::broadcastVolumeFor(Vector3 worldPos) const {
    float d = Vector3Distance(worldPos, cameraTarget_);
    return Clamp(1.0f - d / 40.0f, 0.05f, 1.0f);
}

Vector3 Renderer::playerWorldPos(const Player& p) const {
    float w = static_cast<float>(state_.width()), h = static_cast<float>(state_.height());
    auto wrapf = [](float v, float m) {
        if (m <= 0) return v;
        float r = std::fmod(v, m);
        return r < 0 ? r + m : r;
    };
    return {wrapf(p.fx, w) * kTileSize + p.ox, 0.0f, wrapf(p.fz, h) * kTileSize + p.oz};
}

Vector3 Renderer::tileCenter(float x, float y) const { return {x * kTileSize, 0.0f, y * kTileSize}; }

bool Renderer::projectToScreen(Vector3 world, Vector2& out) const {
    Vector3 fwd = Vector3Normalize(Vector3Subtract(camera_.target, camera_.position));
    if (Vector3DotProduct(Vector3Subtract(world, camera_.position), fwd) <= 0.1f) return false;
    out = GetWorldToScreen(world, camera_);
    return true;
}

bool Renderer::isOverUi(Vector2 mouse) const {
    for (const auto& r : uiRects_)
        if (CheckCollisionPointRec(mouse, r)) return true;
    return false;
}

// -------------------------------------------------------------- game events

void Renderer::processEvents(const std::vector<GameEvent>& events) {
    for (const auto& ev : events) {
        Vector3 c = tileCenter(static_cast<float>(ev.x), static_cast<float>(ev.y));
        Color team = teamColor(ev.teamIndex);

        switch (ev.kind) {
        case GameEventKind::PlayerJoined: {
            const Team* t = ev.teamIndex >= 0 && ev.teamIndex < static_cast<int>(state_.teams().size())
                                ? &state_.teams()[ev.teamIndex] : nullptr;
            log(team, "Player #" + std::to_string(ev.id1) + " joined team " + (t ? t->name : "?"));
            break;
        }
        case GameEventKind::PlayerExpelled:
            break;
        case GameEventKind::PlayerBroadcast:
            log(team, "#" + std::to_string(ev.id1) + " says: " + truncated(ev.text, 90));
            if (audioReady_) {
                // Quieter the farther the broadcast tile is from where the
                // camera is currently looking: a simple spatial audio cue.
                SetSoundVolume(sndBroadcast_, broadcastVolumeFor(c));
                PlaySound(sndBroadcast_);
            }
            break;
        case GameEventKind::IncantationStarted:
            log(Color{255, 225, 120, 255}, "Incantation lvl " + std::to_string(ev.value) + "->" +
                std::to_string(ev.value + 1) + " at (" + std::to_string(ev.x) + "," + std::to_string(ev.y) + ")");
            break;
        case GameEventKind::IncantationEnded:
            if (ev.value) {
                log(Color{130, 255, 150, 255}, "Incantation at (" + std::to_string(ev.x) + "," +
                    std::to_string(ev.y) + ") SUCCEEDED");
            } else {
                log(Color{255, 110, 110, 255}, "Incantation at (" + std::to_string(ev.x) + "," +
                    std::to_string(ev.y) + ") FAILED");
            }
            break;
        case GameEventKind::PlayerFork:
            break;
        case GameEventKind::ResourceTaken:
            break;
        case GameEventKind::ResourceDropped:
            break;
        case GameEventKind::PlayerDied: {
            const Team* t = ev.teamIndex >= 0 && ev.teamIndex < static_cast<int>(state_.teams().size())
                                ? &state_.teams()[ev.teamIndex] : nullptr;
            log(Color{255, 120, 120, 255}, "Player #" + std::to_string(ev.id1) + " (" +
                (t ? t->name : "?") + ") died of hunger");
            if (ev.id1 == selectedPlayer_) clearSelection();
            break;
        }
        case GameEventKind::EggLaid:
            break;
        case GameEventKind::EggHatched:
            break;
        case GameEventKind::EggEnded:
            if (ev.value == 1) {
            } else {
                log(Color{170, 170, 140, 255}, "Egg #" + std::to_string(ev.id1) + " rotted");
            }
            break;
        case GameEventKind::GameOver:
            log(Color{255, 230, 120, 255}, "GAME OVER - team " + ev.text + " wins!");
            break;
        case GameEventKind::ServerMessage:
            log(Color{200, 200, 255, 255}, "Server: " + truncated(ev.text, 100));
            break;
        case GameEventKind::ProtocolError:
            log(Color{255, 170, 90, 255}, ev.text);
            break;
        }
    }
}

// -------------------------------------------------------------------- update

void Renderer::update(float dt) {
    // Camera smoothing / following.
    cameraDist_ += (cameraDistGoal_ - cameraDist_) * std::min(1.0f, dt * 10.0f);
    float cp = std::cos(cameraPitch_);
    Vector3 off = {std::sin(cameraYaw_) * cp, std::sin(cameraPitch_), std::cos(cameraYaw_) * cp};
    if (follow_) {
        if (const Player* p = state_.findPlayer(selectedPlayer_)) {
            Vector3 pp = playerWorldPos(*p);
            cameraTarget_.x += (pp.x - cameraTarget_.x) * std::min(1.0f, dt * 6.0f);
            cameraTarget_.z += (pp.z - cameraTarget_.z) * std::min(1.0f, dt * 6.0f);
        } else {
            follow_ = false;
        }
    }
    camera_.target = cameraTarget_;
    camera_.position = Vector3Add(cameraTarget_, Vector3Scale(off, cameraDist_));
    camera_.up = {0, 1, 0};
    camera_.fovy = 45.0f;
    camera_.projection = CAMERA_PERSPECTIVE;

    // Selection: drop it if the entity is gone, otherwise refresh periodically.
    if (selectedPlayer_ >= 0) {
        if (!state_.findPlayer(selectedPlayer_)) {
            clearSelection();
        } else if ((selectionRefreshTimer_ += dt) > 0.7f) {
            selectionRefreshTimer_ = 0;
            net_.send("pin #" + std::to_string(selectedPlayer_) + "\n");
        }
    } else if (selectedTile_ && (selectionRefreshTimer_ += dt) > 0.7f) {
        selectionRefreshTimer_ = 0;
        net_.send("bct " + std::to_string(selectedTileX_) + " " + std::to_string(selectedTileY_) + "\n");
    }

    // Periodic full-map refresh: the server does not proactively announce
    // resources respawning, so we must re-ask.
    if (net_.isConnected() && state_.hasMap() && !state_.isGameOver()) {
        mapRefreshTimer_ += dt;
        if (mapRefreshTimer_ >= kMapRefreshInterval) {
            mapRefreshTimer_ = 0;
            net_.send("mct\n");
        }
    }

}

// ---------------------------------------------------------------------- input

void Renderer::handleInput(float dt) {
    Vector2 mouse = GetMousePosition();
    bool overUi = isOverUi(mouse);
    if (IsKeyPressed(KEY_L)) showLabels_ = !showLabels_;
    if (IsKeyPressed(KEY_F11)) ToggleFullscreen();
    if (IsKeyPressed(KEY_R)) resetCamera();
    if (IsKeyPressed(KEY_ESCAPE)) clearSelection();
    if (IsKeyPressed(KEY_F) && selectedPlayer_ >= 0) follow_ = !follow_;
    if (IsKeyPressed(KEY_EQUAL) || IsKeyPressed(KEY_KP_ADD)) changeFrequency(+1);
    if (IsKeyPressed(KEY_MINUS) || IsKeyPressed(KEY_KP_SUBTRACT)) changeFrequency(-1);

    if (IsKeyPressed(KEY_TAB) && !state_.players().empty()) {
        const auto& players = state_.players();
        int idx = -1;
        for (size_t i = 0; i < players.size(); i++) if (players[i].id == selectedPlayer_) idx = static_cast<int>(i);
        bool back = IsKeyDown(KEY_LEFT_SHIFT) || IsKeyDown(KEY_RIGHT_SHIFT);
        idx = back ? (idx <= 0 ? static_cast<int>(players.size()) - 1 : idx - 1)
                   : (idx + 1) % static_cast<int>(players.size());
        selectPlayer(players[idx].id);
    }

    float wheel = GetMouseWheelMove();
    if (wheel != 0 && !overUi) cameraDistGoal_ = Clamp(cameraDistGoal_ * (1.0f - wheel * 0.12f), 4.0f, 400.0f);
    if (IsMouseButtonDown(MOUSE_BUTTON_RIGHT) && !overUi) {
        Vector2 d = GetMouseDelta();
        cameraYaw_ -= d.x * 0.006f;
        cameraPitch_ = Clamp(cameraPitch_ + d.y * 0.006f, 0.15f, 1.52f);
    }

    Vector3 fwd = {-std::sin(cameraYaw_), 0, -std::cos(cameraYaw_)};
    Vector3 right = {std::cos(cameraYaw_), 0, -std::sin(cameraYaw_)};
    if (IsMouseButtonDown(MOUSE_BUTTON_MIDDLE)) {
        Vector2 d = GetMouseDelta();
        float k = cameraDist_ * 0.0016f;
        cameraTarget_ = Vector3Add(cameraTarget_, Vector3Scale(right, -d.x * k));
        cameraTarget_ = Vector3Add(cameraTarget_, Vector3Scale(fwd, d.y * k));
        follow_ = false;
    }
    float sp = cameraDist_ * 0.9f * dt * (IsKeyDown(KEY_LEFT_SHIFT) ? 2.0f : 1.0f);
    Vector3 mv = {0, 0, 0};
    if (IsKeyDown(KEY_W) || IsKeyDown(KEY_UP))    mv = Vector3Add(mv, fwd);
    if (IsKeyDown(KEY_S) || IsKeyDown(KEY_DOWN))  mv = Vector3Subtract(mv, fwd);
    if (IsKeyDown(KEY_D) || IsKeyDown(KEY_RIGHT)) mv = Vector3Add(mv, right);
    if (IsKeyDown(KEY_A) || IsKeyDown(KEY_LEFT))  mv = Vector3Subtract(mv, right);
    if (Vector3Length(mv) > 0) { cameraTarget_ = Vector3Add(cameraTarget_, Vector3Scale(mv, sp)); follow_ = false; }
    if (IsKeyDown(KEY_Q)) cameraYaw_ += 1.6f * dt;
    if (IsKeyDown(KEY_E)) cameraYaw_ -= 1.6f * dt;

    if (state_.width() > 0) {
        cameraTarget_.x = Clamp(cameraTarget_.x, -kTileSize, state_.width() * kTileSize);
        cameraTarget_.z = Clamp(cameraTarget_.z, -kTileSize, state_.height() * kTileSize);
    }

    hoverTile_ = false;
    hoverPlayer_ = -1;
    if (!state_.hasMap() || overUi) return;

    Ray ray = GetMouseRay(mouse, camera_);
    if (std::fabs(ray.direction.y) > 1e-4f) {
        float t = -ray.position.y / ray.direction.y;
        if (t > 0) {
            Vector3 h = Vector3Add(ray.position, Vector3Scale(ray.direction, t));
            int tx = static_cast<int>(std::floor(h.x / kTileSize + 0.5f));
            int ty = static_cast<int>(std::floor(h.z / kTileSize + 0.5f));
            if (tx >= 0 && ty >= 0 && tx < state_.width() && ty < state_.height()) {
                hoverTile_ = true;
                hoverTileX_ = tx;
                hoverTileY_ = ty;
            }
        }
    }

    float best = 1e9f;
    for (const auto& p : state_.players()) {
        if (p.dyingT > 0) continue;
        float s = p.scale();
        Vector3 c = playerWorldPos(p);
        c.y = 0.7f * s;
        RayCollision rc = GetRayCollisionSphere(ray, c, 0.72f * s);
        if (rc.hit && rc.distance < best) { best = rc.distance; hoverPlayer_ = p.id; }
    }

    if (IsMouseButtonPressed(MOUSE_BUTTON_LEFT)) {
        if (hoverPlayer_ >= 0) selectPlayer(hoverPlayer_);
        else if (hoverTile_) selectTile(hoverTileX_, hoverTileY_);
        else clearSelection();
    }
}

// ------------------------------------------------------------------- 3D draw

void Renderer::drawResource(int type, Vector3 p, float scale) const {
    const float size = 0.35f * scale;
    DrawCube({p.x, p.y + size * 0.5f, p.z}, size, size, size, kResColor[type]);
}

void Renderer::drawGround() 
{
    int W = state_.width(), H = state_.height();
    float cx = (W - 1) * 0.5f * kTileSize, cz = (H - 1) * 0.5f * kTileSize;
    DrawPlane({cx, -0.7f, cz}, {4000, 4000}, Color{34, 52, 72, 255});
    DrawCube({cx, -0.22f, cz}, W * kTileSize + 0.3f, 0.3f, H * kTileSize + 0.3f, Color{78, 58, 44, 255});

    rlDisableBackfaceCulling();
    const float h = kTileSize * 0.5f - 0.045f;
    for (int y = 0; y < H; y++) {
        for (int x = 0; x < W; x++) {
            Color c = ((x + y) & 1) ? Color{88, 154, 92, 255} : Color{78, 142, 84, 255};
            float px = x * kTileSize, pz = y * kTileSize;
            rlCheckRenderBatchLimit(4);
            rlBegin(RL_QUADS);
            rlColor4ub(c.r, c.g, c.b, 255);
            rlVertex3f(px - h, 0, pz - h); rlVertex3f(px - h, 0, pz + h);
            rlVertex3f(px + h, 0, pz + h); rlVertex3f(px + h, 0, pz - h);
            rlEnd();
        }
    }
    rlEnableBackfaceCulling();
}

void Renderer::drawResources() {
    for (int y = 0; y < state_.height(); y++) {
        for (int x = 0; x < state_.width(); x++) {
            const Tile* tile = state_.tileAt(x, y);
            if (!tile) continue;
            Vector3 c = tileCenter(static_cast<float>(x), static_cast<float>(y));
            for (int r = 0; r < kResourceCount; r++) {
                if (tile->resources[r] <= 0) continue;
                float a = r * (2 * kPi / kResourceCount) + 0.4f;
                drawResource(r, {c.x + std::cos(a) * 0.62f, 0.0f, c.z + std::sin(a) * 0.62f}, 1.0f);
            }
        }
    }
}

void Renderer::drawPlayer(const Player& p) const {
    Color tc = teamColor(p.team);
    float s = p.scale();
    Vector3 pos = playerWorldPos(p);
    DrawCylinder({pos.x, 0.6f * s, pos.z}, 0.45f * s, 0.45f * s, 1.2f * s, 12, tc);
    DrawSphere({pos.x, 1.45f * s, pos.z}, 0.35f * s, tc);
}

void Renderer::drawEgg(const Egg& e) const {
    Color tc = teamColor(e.team);
    Color body = mixColor(Color{250, 245, 225, 255}, tc, 0.42f);
    if (e.hatched) body = mixColor(body, Color{255, 240, 140, 255}, 0.4f);
    if (e.state == EggState::Rotten) body = mixColor(body, Color{90, 100, 70, 255}, 0.7f);

    Vector3 c = tileCenter(static_cast<float>(e.x), static_cast<float>(e.y));
    float a = static_cast<float>((e.id * 37) % 360) * kPi / 180.0f;
    float r = 0.35f + static_cast<float>((e.id * 17) % 45) / 100.0f;
    Vector3 pos = {c.x + std::cos(a) * r, 0, c.z + std::sin(a) * r};

    DrawSphere({pos.x, 0.42f, pos.z}, 0.32f, body);
}

void Renderer::drawWorld() {
    rlSetClipPlanes(0.1, 3000.0);
    BeginMode3D(camera_);
    if (state_.hasMap()) {
        drawGround();
        drawResources();
        for (const auto& e : state_.eggs()) drawEgg(e);
        for (const auto& p : state_.players()) drawPlayer(p);
        if (selectedTile_ || hoverTile_) {
            int x = selectedTile_ ? selectedTileX_ : hoverTileX_;
            int y = selectedTile_ ? selectedTileY_ : hoverTileY_;
            Color color = selectedTile_ ? YELLOW : SKYBLUE;
            DrawCubeWires(tileCenter(static_cast<float>(x), static_cast<float>(y)),
                          kTileSize, 0.05f, kTileSize, color);
        }
        if (const Player* p = selectedPlayer_ >= 0 ? state_.findPlayer(selectedPlayer_) : nullptr) {
            Vector3 pos = playerWorldPos(*p);
            DrawCylinderWires({pos.x, 0.03f, pos.z}, 0.55f * p->scale(), 0.55f * p->scale(),
                              0.05f, 16, YELLOW);
        }
    }
    EndMode3D();
}

// ------------------------------------------------------------------- 2D draw

void Renderer::drawLabels() {
    auto drawTileNumbers = [&](int x, int y) {
        const Tile* tile = state_.tileAt(x, y);
        if (!tile) return;
        Vector3 c = tileCenter(static_cast<float>(x), static_cast<float>(y));
        for (int r = 0; r < kResourceCount; r++) {
            if (tile->resources[r] <= 0) continue;
            float a = r * (2 * kPi / kResourceCount) + 0.4f;
            Vector2 s;
            if (!projectToScreen({c.x + std::cos(a) * 0.62f, 0.75f, c.z + std::sin(a) * 0.62f}, s)) continue;
            const char* txt = TextFormat("%d", tile->resources[r]);
            int w = MeasureText(txt, 14);
            DrawRectangle(static_cast<int>(s.x) - w / 2 - 3, static_cast<int>(s.y) - 2, w + 6, 17, Color{0, 0, 0, 170});
            DrawText(txt, static_cast<int>(s.x) - w / 2, static_cast<int>(s.y), 14, kResColor[r]);
        }
    };

    if (hoverTile_) drawTileNumbers(hoverTileX_, hoverTileY_);
    if (selectedTile_) drawTileNumbers(selectedTileX_, selectedTileY_);

    for (const auto& inc : state_.incantations()) {
        Vector2 s;
        Vector3 c = tileCenter(static_cast<float>(inc.x), static_cast<float>(inc.y));
        if (projectToScreen({c.x, 3.2f, c.z}, s))
            drawTextCentered(static_cast<int>(s.x), static_cast<int>(s.y), 14, Color{255, 230, 130, 255},
                             "Elevation %d > %d", inc.level, inc.level + 1);
    }

    if (!showLabels_) return;
    for (const auto& p : state_.players()) {
        if (p.dyingT > 0) continue;
        Vector3 pos = playerWorldPos(p);
        bool emph = p.id == selectedPlayer_ || p.id == hoverPlayer_;
        if (!emph && Vector3Distance(pos, camera_.position) > 65.0f) continue;
        Vector2 s;
        if (!projectToScreen({pos.x, 2.05f * p.scale(), pos.z}, s)) continue;
        Color tc = teamColor(p.team);
        DrawCircle(static_cast<int>(s.x), static_cast<int>(s.y), 10, Color{0, 0, 0, 170});
        DrawCircle(static_cast<int>(s.x), static_cast<int>(s.y), 9, tc);
        int lum = tc.r * 3 + tc.g * 6 + tc.b;
        drawTextCentered(static_cast<int>(s.x), static_cast<int>(s.y) - 6, 13, lum > 1400 ? BLACK : WHITE, "%d", p.level);
        if (emph) {
            const Team* t = p.team >= 0 && p.team < static_cast<int>(state_.teams().size()) ? &state_.teams()[p.team] : nullptr;
            drawTextCentered(static_cast<int>(s.x), static_cast<int>(s.y) - 30, 14, WHITE, "#%d  %s", p.id, t ? t->name.c_str() : "?");
        }
    }
}

void Renderer::drawTeamsPanel() {
    const auto& teams = state_.teams();
    int rows = teams.empty() ? 1 : static_cast<int>(teams.size());
    Rectangle r = {10, 42, 285, 30.0f + rows * 38.0f};
    DrawRectangleRec(r, Color{12, 16, 28, 205});
    DrawRectangleLinesEx(r, 1, Color{90, 110, 150, 210});
    uiRects_.push_back(r);

    drawText(20, 50, 15, Color{170, 190, 220, 255}, "TEAMS");
    if (teams.empty()) {
        drawText(20, 74, 14, GRAY, "waiting for server...");
        return;
    }
    for (size_t i = 0; i < teams.size(); i++) {
        const Team& t = teams[i];
        int y = 72 + static_cast<int>(i) * 38;
        DrawRectangle(20, y + 2, 14, 14, teamColor(t.colorIndex));
        drawText(42, y, 16, WHITE, "%.24s", t.name.c_str());
        drawText(42, y + 18, 13, Color{190, 200, 220, 255}, "players %d  max lvl %d  lvl8 %d/6  eggs %d",
                t.count, t.maxLevel, t.level8Count, t.eggCount);
    }
}

void Renderer::drawSelectionPanel() {
    int sw = GetScreenWidth();
    if (const Player* sp = selectedPlayer_ >= 0 ? state_.findPlayer(selectedPlayer_) : nullptr) {
        Rectangle r = {sw - 300.0f, 42, 290, 270};
        DrawRectangleRec(r, Color{12, 16, 28, 205});
        DrawRectangleLinesEx(r, 1, Color{90, 110, 150, 210});
        uiRects_.push_back(r);

        DrawRectangle(static_cast<int>(r.x) + 12, 54, 14, 14, teamColor(sp->team));
        drawText(static_cast<int>(r.x) + 34, 51, 18, WHITE, "Player #%d", sp->id);
        const Team* t = sp->team >= 0 && sp->team < static_cast<int>(state_.teams().size()) ? &state_.teams()[sp->team] : nullptr;
        drawText(static_cast<int>(r.x) + 12, 78, 15, Color{190, 200, 220, 255}, "Team   %s", t ? t->name.c_str() : "?");
        drawText(static_cast<int>(r.x) + 12, 98, 15, Color{190, 200, 220, 255}, "Level  %d%s", sp->level, sp->incanting ? "   (incanting)" : "");
        int orient = sp->orientation >= 1 && sp->orientation <= 4 ? sp->orientation : 0;
        drawText(static_cast<int>(r.x) + 12, 118, 15, Color{190, 200, 220, 255}, "Tile   (%d, %d)  facing %s", sp->x, sp->y, kOrientName[orient]);
        drawText(static_cast<int>(r.x) + 12, 142, 14, Color{170, 190, 220, 255}, "INVENTORY%s", follow_ ? "        [following]" : "");
        for (int i = 0; i < kResourceCount; i++) {
            int y = 162 + i * 20;
            DrawCircle(static_cast<int>(r.x) + 18, y + 8, 6, kResColor[i]);
            drawText(static_cast<int>(r.x) + 30, y, 15, RAYWHITE, "%-10s", kResName[i]);
            drawText(static_cast<int>(r.x) + 130, y, 15, kResColor[i], "%d", sp->hasInventory ? sp->inventory[i] : 0);
        }
        return;
    }

    if (selectedTile_ && state_.tileAt(selectedTileX_, selectedTileY_)) {
        Rectangle r = {sw - 300.0f, 42, 290, 262};
        DrawRectangleRec(r, Color{12, 16, 28, 205});
        DrawRectangleLinesEx(r, 1, Color{90, 110, 150, 210});
        uiRects_.push_back(r);

        drawText(static_cast<int>(r.x) + 12, 51, 18, WHITE, "Tile (%d, %d)", selectedTileX_, selectedTileY_);
        const Tile* tile = state_.tileAt(selectedTileX_, selectedTileY_);
        for (int i = 0; i < kResourceCount; i++) {
            int y = 80 + i * 20;
            DrawCircle(static_cast<int>(r.x) + 18, y + 8, 6, kResColor[i]);
            drawText(static_cast<int>(r.x) + 30, y, 15, RAYWHITE, "%-10s", kResName[i]);
            drawText(static_cast<int>(r.x) + 130, y, 15, kResColor[i], "%d", tile->resources[i]);
        }
        int np = 0, ne = 0;
        std::string ids;
        for (const auto& p : state_.players())
            if (p.x == selectedTileX_ && p.y == selectedTileY_) {
                if (np < 8) ids += "#" + std::to_string(p.id) + " ";
                np++;
            }
        for (const auto& e : state_.eggs())
            if (e.x == selectedTileX_ && e.y == selectedTileY_) ne++;
        drawText(static_cast<int>(r.x) + 12, 228, 14, Color{190, 200, 220, 255}, "players: %d  eggs: %d", np, ne);
        drawText(static_cast<int>(r.x) + 12, 246, 13, Color{170, 180, 200, 255}, "%.34s", ids.c_str());
    }
}

void Renderer::drawOverlays() {
    int sw = GetScreenWidth(), sh = GetScreenHeight();

    if (!net_.isConnected() || !state_.hasMap()) {
        Rectangle r = {sw / 2.0f - 230, sh / 2.0f - 40, 460, 80};
        DrawRectangleRec(r, Color{12, 16, 28, 205});
        DrawRectangleLinesEx(r, 1, Color{90, 110, 150, 210});
        uiRects_.push_back(r);
        std::string msg = net_.isConnected() ? "Waiting for the server..." : net_.lastError();
        if (msg.empty()) msg = "Not connected";
        drawTextCentered(sw / 2, static_cast<int>(r.y) + 14, 18, WHITE, "%s", msg.c_str());
        drawTextCentered(sw / 2, static_cast<int>(r.y) + 44, 14, GRAY, net_.isConnected() ? "handshake in progress" : "Press C to retry");
    }

    if (state_.isGameOver()) {
        Color c = state_.winnerTeamIndex() >= 0 ? teamColor(state_.winnerTeamIndex()) : Color{255, 220, 100, 255};
        DrawRectangle(0, sh / 2 - 70, sw, 120, Color{0, 0, 0, 150});
        drawTextCentered(sw / 2, sh / 2 - 55, 22, WHITE, "GAME OVER");
        drawTextCentered(sw / 2, sh / 2 - 20, 44, c, "Team %s wins!", state_.winnerName().c_str());
    }
}

void Renderer::drawHud() {
    int sw = GetScreenWidth();
    double now = GetTime();
    uiRects_.clear();

    Rectangle top = {0, 0, static_cast<float>(sw), 32};
    DrawRectangleRec(top, Color{12, 16, 28, 205});
    DrawRectangleLinesEx(top, 1, Color{90, 110, 150, 210});
    uiRects_.push_back(top);

    drawText(12, 7, 20, Color{255, 210, 90, 255}, "ZAPPY");
    drawText(92, 10, 14, RAYWHITE, "map %dx%d    time unit %d    players %d    eggs %d    %d FPS",
            state_.width(), state_.height(), state_.timeUnit(),
            static_cast<int>(state_.players().size()), static_cast<int>(state_.eggs().size()), GetFPS());
    {
        std::string status = net_.isConnected() ? "* connected" : "DISCONNECTED - press C to reconnect";
        int w = MeasureText(status.c_str(), 14);
        drawText(sw - 90 - w, 10, 14, net_.isConnected() ? Color{110, 240, 130, 255} : Color{255, 110, 110, 255}, "%s", status.c_str());
    }
    if (audioReady_)
        drawText(sw - 160, 10, 14, Color{140, 220, 170, 255}, "broadcast audio");

    drawTeamsPanel();
    drawSelectionPanel();

    int shown = 0;
    for (auto it = log_.rbegin(); it != log_.rend() && shown < 9; ++it) {
        float age = static_cast<float>(now - it->time);
        if (age > 14.0f) break;
        float a = age > 10.0f ? 1.0f - (age - 10.0f) / 4.0f : 1.0f;
        Color c = it->color;
        c.a = static_cast<unsigned char>(255 * a);
        int y = GetScreenHeight() - 24 - shown * 18;
        DrawRectangle(10, y - 1, MeasureText(it->text.c_str(), 14) + 12, 18, Color{0, 0, 0, static_cast<unsigned char>(120 * a)});
        DrawText(it->text.c_str(), 16, y, 14, c);
        shown++;
    }

    drawOverlays();
}

void Renderer::draw() {
    drawWorld();
    drawLabels();
    drawHud();
}

} // namespace zappy
