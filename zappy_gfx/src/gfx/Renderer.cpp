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

const Color kSkyTop = {22, 32, 64, 255};
const Color kSkyBot = {84, 116, 160, 255};

float frand(float a, float b) { return a + (b - a) * (GetRandomValue(0, 10000) / 10000.0f); }

float hashf(unsigned a) {
    a ^= a << 13; a *= 0x9E3779B1u; a ^= a >> 15; a *= 0x85EBCA6Bu; a ^= a >> 13;
    return (a & 0xFFFF) / 65535.0f;
}

float easeOutBack(float x) {
    const float c1 = 1.70158f, c3 = c1 + 1.0f;
    x = Clamp(x, 0, 1) - 1.0f;
    return 1.0f + c3 * x * x * x + c1 * x * x;
}

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

const char* VS_SRC =
    "#version 330\n"
    "in vec3 vertexPosition;\n"
    "in vec2 vertexTexCoord;\n"
    "in vec4 vertexColor;\n"
    "uniform mat4 mvp;\n"
    "out vec3 fragPos;\n"
    "out vec4 fragColor;\n"
    "void main(){\n"
    "  fragPos = vertexPosition;\n"
    "  fragColor = vertexColor;\n"
    "  gl_Position = mvp*vec4(vertexPosition,1.0);\n"
    "}\n";
const char* FS_SRC =
    "#version 330\n"
    "in vec3 fragPos;\n"
    "in vec4 fragColor;\n"
    "uniform vec3 camPos;\n"
    "uniform vec3 fogColor;\n"
    "out vec4 finalColor;\n"
    "void main(){\n"
    "  vec3 n = normalize(cross(dFdx(fragPos), dFdy(fragPos)));\n"
    "  vec3 v = normalize(camPos - fragPos);\n"
    "  if (dot(n, v) < 0.0) n = -n;\n"
    "  vec3 L = normalize(vec3(0.45, 1.0, 0.35));\n"
    "  float diff = max(dot(n, L), 0.0);\n"
    "  float rim = pow(1.0 - max(dot(n, v), 0.0), 3.0) * 0.12;\n"
    "  vec3 light = vec3(0.40, 0.44, 0.54) + vec3(1.0, 0.96, 0.86) * diff * 0.78 + rim;\n"
    "  vec3 col = fragColor.rgb * light;\n"
    "  float d = length(camPos - fragPos);\n"
    "  float f = clamp((d - 70.0) / 260.0, 0.0, 1.0);\n"
    "  col = mix(col, fogColor, f * f);\n"
    "  finalColor = vec4(col, fragColor.a);\n"
    "}\n";

} // namespace

// ------------------------------------------------------------------ lifecycle

bool Renderer::init(int width, int height, const char* title) {
    SetConfigFlags(FLAG_MSAA_4X_HINT | FLAG_WINDOW_RESIZABLE | FLAG_VSYNC_HINT);
    InitWindow(width, height, title);
    if (!IsWindowReady()) return false;

    SetExitKey(KEY_NULL);
    SetTargetFPS(60);
    SetWindowMinSize(800, 500);

    shader_ = LoadShaderFromMemory(VS_SRC, FS_SRC);
    useShader_ = shader_.id != rlGetShaderIdDefault() && shader_.id != 0;
    if (useShader_) {
        locCamPos_ = GetShaderLocation(shader_, "camPos");
        locFogColor_ = GetShaderLocation(shader_, "fogColor");
        float fog[3] = {kSkyBot.r / 255.0f, kSkyBot.g / 255.0f, kSkyBot.b / 255.0f};
        SetShaderValue(shader_, locFogColor_, fog, SHADER_UNIFORM_VEC3);
    }

    resetCamera();
    loadSounds();
    return true;
}

void Renderer::shutdown() {
    unloadSounds();
    if (useShader_) UnloadShader(shader_);
    CloseWindow();
}

bool Renderer::shouldClose() const { return WindowShouldClose(); }

void Renderer::beginFrame() {
    BeginDrawing();
    ClearBackground(kSkyBot);
    DrawRectangleGradientV(0, 0, GetScreenWidth(), GetScreenHeight(), kSkyTop, kSkyBot);
}

void Renderer::endFrame() { EndDrawing(); }

void Renderer::takeScreenshot(const char* path) { TakeScreenshot(path); }

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

void Renderer::spawnBurst(Vector3 pos, Color color, int count, float speed, float up, float life, float size) {
    for (int i = 0; i < count; i++) {
        float a = frand(0, 2 * kPi), r = frand(0.3f, 1.0f) * speed;
        Particle p;
        p.pos = pos;
        p.vel = {std::cos(a) * r, frand(0.3f, 1.0f) * up, std::sin(a) * r};
        p.color = color;
        p.life = p.maxLife = life * frand(0.6f, 1.0f);
        p.size = size * frand(0.7f, 1.3f);
        p.gravity = 6.0f;
        particles_.push_back(p);
    }
}

void Renderer::spawnRing(Vector3 pos, Color color, float r0, float r1, float duration) {
    rings_.push_back({pos, color, 0, duration, r0, r1});
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

// -------------------------------------------------------------------- audio
//
// Every sound here is synthesized in memory, not loaded from disk, so there
// is no asset file that can go missing or need copying in the Makefile.

Sound Renderer::makeMelody(const std::vector<std::pair<float, float>>& notes, float volume) {
    constexpr int sampleRate = 44100;
    int totalFrames = 0;
    for (const auto& n : notes) totalFrames += static_cast<int>(n.second * sampleRate);
    if (totalFrames <= 0) totalFrames = 1;

    std::vector<short> samples(static_cast<size_t>(totalFrames), 0);
    int offset = 0;
    for (const auto& n : notes) {
        float freq = n.first;
        int frames = static_cast<int>(n.second * sampleRate);
        for (int i = 0; i < frames && offset + i < totalFrames; i++) {
            float t = static_cast<float>(i) / sampleRate;
            // Half-sine envelope per note: fades in and out smoothly, so
            // consecutive notes (and the loop point) never click/pop.
            float env = std::sin(kPi * static_cast<float>(i) / frames);
            float s = (freq > 0.0f) ? std::sin(2 * kPi * freq * t) * env * volume : 0.0f;
            samples[static_cast<size_t>(offset + i)] = static_cast<short>(s * 32000);
        }
        offset += frames;
    }

    Wave wave{};
    wave.frameCount = static_cast<unsigned int>(totalFrames);
    wave.sampleRate = sampleRate;
    wave.sampleSize = 16;
    wave.channels = 1;
    wave.data = samples.data();
    return LoadSoundFromWave(wave);   // copies samples into raylib's own buffer
}

Sound Renderer::makeTone(float freqHz, float durationSec, float volume) {
    return makeMelody({{freqHz, durationSec}}, volume);
}

void Renderer::loadSounds() {
    InitAudioDevice();
    audioReady_ = IsAudioDeviceReady();
    if (!audioReady_) return;   // headless/no sound card: everything below is skipped at call sites

    sndBroadcast_     = makeTone(880.0f, 0.18f, 0.5f);
    sndJoin_          = makeTone(520.0f, 0.12f, 0.4f);
    sndDeath_         = makeTone(140.0f, 0.35f, 0.5f);
    sndIncantSuccess_ = makeMelody({{523.25f, 0.12f}, {659.25f, 0.12f}, {783.99f, 0.20f}}, 0.5f);   // C-E-G
    sndIncantFail_    = makeTone(90.0f, 0.25f, 0.45f);
    // Short original victory fanfare (not based on any existing composition).
    sndVictory_       = makeMelody({{523.25f, 0.15f}, {523.25f, 0.15f}, {659.25f, 0.15f},
                                    {783.99f, 0.30f}, {659.25f, 0.15f}, {987.77f, 0.45f}}, 0.55f);
    // Soft looping ambient arpeggio for the exploration phase.
    sndAmbientLoop_   = makeMelody({{261.63f, 0.6f}, {0, 0.2f}, {329.63f, 0.6f},
                                    {0, 0.2f}, {392.00f, 0.6f}, {0, 0.4f}}, 0.16f);
}

void Renderer::unloadSounds() {
    if (!audioReady_) return;
    UnloadSound(sndBroadcast_);
    UnloadSound(sndJoin_);
    UnloadSound(sndDeath_);
    UnloadSound(sndIncantSuccess_);
    UnloadSound(sndIncantFail_);
    UnloadSound(sndVictory_);
    UnloadSound(sndAmbientLoop_);
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
            spawnBurst({c.x, 0.2f, c.z}, team, 16, 2.0f, 4.0f, 0.9f, 0.12f);
            spawnRing(c, team, 0.2f, 1.4f, 0.7f);
            log(team, "Player #" + std::to_string(ev.id1) + " joined team " + (t ? t->name : "?"));
            if (audioReady_) PlaySound(sndJoin_);
            break;
        }
        case GameEventKind::PlayerExpelled:
            spawnRing({c.x, 0, c.z}, Color{255, 90, 60, 255}, 0.3f, 2.2f, 0.5f);
            spawnBurst({c.x, 0.8f, c.z}, Color{255, 140, 80, 255}, 10, 3.0f, 2.0f, 0.6f, 0.1f);
            break;
        case GameEventKind::PlayerBroadcast:
            spawnRing(c, team, 0.4f, 9.0f, 1.6f);
            spawnRing(c, team, 0.2f, 6.0f, 1.2f);
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
                spawnBurst({c.x, 0.3f, c.z}, Color{255, 230, 110, 255}, 70, 3.5f, 9.0f, 1.6f, 0.13f);
                spawnBurst({c.x, 0.3f, c.z}, Color{120, 255, 160, 255}, 40, 2.5f, 7.0f, 1.4f, 0.11f);
                spawnRing(c, Color{255, 230, 110, 255}, 0.3f, 4.0f, 1.0f);
                log(Color{130, 255, 150, 255}, "Incantation at (" + std::to_string(ev.x) + "," +
                    std::to_string(ev.y) + ") SUCCEEDED");
                if (audioReady_) PlaySound(sndIncantSuccess_);
            } else {
                spawnBurst({c.x, 0.3f, c.z}, Color{255, 70, 70, 255}, 40, 2.5f, 3.0f, 1.2f, 0.12f);
                spawnRing(c, Color{255, 70, 70, 255}, 0.3f, 2.5f, 0.8f);
                log(Color{255, 110, 110, 255}, "Incantation at (" + std::to_string(ev.x) + "," +
                    std::to_string(ev.y) + ") FAILED");
                if (audioReady_) PlaySound(sndIncantFail_);
            }
            break;
        case GameEventKind::PlayerFork:
            spawnRing(c, Color{255, 255, 220, 255}, 0.2f, 1.2f, 0.6f);
            break;
        case GameEventKind::ResourceTaken:
            for (int i = 0; i < 6; i++)
                particles_.push_back({
                    {c.x + frand(-.3f, .3f), 0.2f, c.z + frand(-.3f, .3f)},
                    {0, frand(2.0f, 3.5f), 0}, kResColor[ev.value], 0.7f, 0.7f, 0.14f, 0.0f});
            break;
        case GameEventKind::ResourceDropped:
            for (int i = 0; i < 6; i++)
                particles_.push_back({
                    {c.x + frand(-.3f, .3f), 1.6f, c.z + frand(-.3f, .3f)},
                    {frand(-.5f, .5f), 0.5f, frand(-.5f, .5f)}, kResColor[ev.value], 0.8f, 0.8f, 0.14f, 6.0f});
            break;
        case GameEventKind::PlayerDied: {
            const Team* t = ev.teamIndex >= 0 && ev.teamIndex < static_cast<int>(state_.teams().size())
                                ? &state_.teams()[ev.teamIndex] : nullptr;
            spawnBurst({c.x, 0.8f, c.z}, Color{255, 60, 60, 255}, 30, 2.5f, 5.0f, 1.2f, 0.13f);
            spawnRing(c, Color{255, 60, 60, 255}, 0.3f, 2.5f, 0.9f);
            log(Color{255, 120, 120, 255}, "Player #" + std::to_string(ev.id1) + " (" +
                (t ? t->name : "?") + ") died of hunger");
            if (ev.id1 == selectedPlayer_) clearSelection();
            if (audioReady_) PlaySound(sndDeath_);
            break;
        }
        case GameEventKind::EggLaid:
            spawnBurst({c.x, 0.3f, c.z}, Color{255, 255, 230, 255}, 8, 1.5f, 3.0f, 0.7f, 0.1f);
            break;
        case GameEventKind::EggHatched:
            spawnBurst({c.x, 0.4f, c.z}, Color{255, 240, 150, 255}, 10, 1.5f, 3.0f, 0.7f, 0.1f);
            break;
        case GameEventKind::EggEnded:
            if (ev.value == 1) {
                spawnBurst({c.x, 0.4f, c.z}, Color{140, 255, 170, 255}, 22, 2.2f, 4.0f, 0.9f, 0.11f);
            } else {
                spawnBurst({c.x, 0.4f, c.z}, Color{110, 130, 90, 255}, 16, 1.8f, 2.5f, 0.9f, 0.11f);
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

    // Continuous incantation sparkles.
    for (const auto& inc : state_.incantations()) {
        Vector3 c = tileCenter(static_cast<float>(inc.x), static_cast<float>(inc.y));
        int sparks = static_cast<int>(dt * 40.0f) + (GetRandomValue(0, 100) < 40 ? 1 : 0);
        for (int k = 0; k < sparks; k++) {
            float a = frand(0, 2 * kPi), r = frand(0.2f, 1.2f);
            particles_.push_back({
                {c.x + std::cos(a) * r, 0.1f, c.z + std::sin(a) * r},
                {0, frand(1.5f, 4.0f), 0}, Color{255, 225, 120, 255}, 1.2f, 1.2f, 0.09f, 0.0f});
        }
    }

    // Background music: a soft ambient loop while exploring, restarted
    // manually each time it finishes (Sound has no native seamless-loop
    // flag; the half-sine envelope on every note keeps restarts click-free).
    // It stops the moment the game ends, replaced by a one-shot fanfare.
    if (audioReady_ && musicEnabled_ && state_.hasMap() && !state_.isGameOver()) {
        if (!IsSoundPlaying(sndAmbientLoop_)) PlaySound(sndAmbientLoop_);
    }
    if (state_.isGameOver() && !previousGameOver_) {
        if (audioReady_) {
            StopSound(sndAmbientLoop_);
            PlaySound(sndVictory_);
        }
    }
    previousGameOver_ = state_.isGameOver();

    // Fireworks once the game is over.
    if (state_.isGameOver() && state_.hasMap()) {
        fireworkTimer_ -= dt;
        if (fireworkTimer_ <= 0) {
            fireworkTimer_ = 0.18f;
            Color fc = state_.winnerTeamIndex() >= 0 ? teamColor(state_.winnerTeamIndex()) : Color{255, 220, 100, 255};
            if (GetRandomValue(0, 1))
                fc = Color{static_cast<unsigned char>(GetRandomValue(120, 255)),
                          static_cast<unsigned char>(GetRandomValue(120, 255)),
                          static_cast<unsigned char>(GetRandomValue(120, 255)), 255};
            Vector3 o = {frand(0, state_.width() * kTileSize), frand(6, 14), frand(0, state_.height() * kTileSize)};
            for (int k = 0; k < 40; k++) {
                float a = frand(0, 2 * kPi), b = frand(-1, 1), r = std::sqrt(1 - b * b) * 4.0f;
                particles_.push_back({o, {std::cos(a) * r, b * 4.0f, std::sin(a) * r},
                                      fc, 1.6f, 1.6f, 0.14f, 3.0f});
            }
        }
    }

    // Particle / ring decay.
    for (size_t i = 0; i < particles_.size(); ) {
        Particle& p = particles_[i];
        p.life -= dt;
        p.vel.y -= p.gravity * dt;
        p.pos = Vector3Add(p.pos, Vector3Scale(p.vel, dt));
        if (p.life <= 0 || (p.pos.y < 0 && p.gravity > 0)) {
            particles_[i] = particles_.back();
            particles_.pop_back();
            continue;
        }
        i++;
    }
    for (size_t i = 0; i < rings_.size(); ) {
        rings_[i].t += dt;
        if (rings_[i].t >= rings_[i].duration) {
            rings_[i] = rings_.back();
            rings_.pop_back();
            continue;
        }
        i++;
    }
}

// ---------------------------------------------------------------------- input

namespace {
Rectangle minimapRect(const GameState& state) {
    if (!state.hasMap()) return {0, 0, 0, 0};
    float cell = std::min(200.0f / state.width(), 200.0f / state.height());
    if (cell > 10) cell = 10;
    float mw = cell * state.width(), mh = cell * state.height();
    return {GetScreenWidth() - mw - 14, GetScreenHeight() - mh - 14, mw, mh};
}
} // namespace

void Renderer::handleInput(float dt) {
    Vector2 mouse = GetMousePosition();
    bool overUi = isOverUi(mouse);
    Rectangle mm = showMinimap_ ? minimapRect(state_) : Rectangle{0, 0, 0, 0};

    if (IsKeyPressed(KEY_H)) showHelp_ = !showHelp_;
    if (IsKeyPressed(KEY_M)) showMinimap_ = !showMinimap_;
    if (IsKeyPressed(KEY_L)) showLabels_ = !showLabels_;
    if (IsKeyPressed(KEY_F11)) ToggleFullscreen();
    if (IsKeyPressed(KEY_R)) resetCamera();
    if (IsKeyPressed(KEY_ESCAPE)) { clearSelection(); showHelp_ = false; }
    if (IsKeyPressed(KEY_F) && selectedPlayer_ >= 0) follow_ = !follow_;
    if (IsKeyPressed(KEY_EQUAL) || IsKeyPressed(KEY_KP_ADD)) changeFrequency(+1);
    if (IsKeyPressed(KEY_MINUS) || IsKeyPressed(KEY_KP_SUBTRACT)) changeFrequency(-1);
    if (IsKeyPressed(KEY_N)) {
        musicEnabled_ = !musicEnabled_;
        if (!musicEnabled_ && audioReady_) StopSound(sndAmbientLoop_);
    }

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

    if (showMinimap_ && mm.width > 0) {
        if (IsMouseButtonPressed(MOUSE_BUTTON_LEFT) && CheckCollisionPointRec(mouse, mm)) minimapDrag_ = true;
        if (IsMouseButtonReleased(MOUSE_BUTTON_LEFT)) minimapDrag_ = false;
        if (minimapDrag_) {
            float cell = mm.width / state_.width();
            cameraTarget_.x = Clamp((mouse.x - mm.x) / cell, 0, static_cast<float>(state_.width())) * kTileSize - kTileSize * 0.5f;
            cameraTarget_.z = Clamp((mouse.y - mm.y) / cell, 0, static_cast<float>(state_.height())) * kTileSize - kTileSize * 0.5f;
            follow_ = false;
        }
    }

    hoverTile_ = false;
    hoverPlayer_ = -1;
    if (!state_.hasMap() || overUi || minimapDrag_) return;

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

void Renderer::drawResource(int type, Vector3 p, float t, float scale, unsigned seed) const {
    Color c = kResColor[type];
    float s = scale;
    float bob = 0.04f * std::sin(t * 2.0f + seed);
    rlPushMatrix();
    rlTranslatef(p.x, p.y + bob, p.z);
    rlRotatef(t * 40.0f + seed * 37.0f, 0, 1, 0);
    switch (type) {
    case 0:  // food: apple
        DrawSphereEx({0, 0.15f * s, 0}, 0.15f * s, 8, 8, c);
        DrawCube({0, 0.32f * s, 0}, 0.03f * s, 0.08f * s, 0.03f * s, Color{90, 60, 30, 255});
        break;
    case 1:  // linemate: cube
        DrawCube({0, 0.14f * s, 0}, 0.24f * s, 0.24f * s, 0.24f * s, c);
        break;
    case 2:  // deraumere: bipyramid
        DrawCylinder({0, 0.2f * s, 0}, 0, 0.17f * s, 0.2f * s, 4, c);
        DrawCylinder({0, 0.0f, 0}, 0.17f * s, 0, 0.2f * s, 4, c);
        break;
    case 3:  // sibur: hex prism
        DrawCylinder({0, 0.02f, 0}, 0.13f * s, 0.13f * s, 0.3f * s, 6, c);
        break;
    case 4:  // mendiane: pyramid
        DrawCylinder({0, 0.02f, 0}, 0.0f, 0.18f * s, 0.34f * s, 4, c);
        break;
    case 5:  // phiras: low-poly gem
        DrawSphereEx({0, 0.16f * s, 0}, 0.16f * s, 4, 5, c);
        break;
    default: // thystame: tall double pyramid
        DrawCylinder({0, 0.22f * s, 0}, 0, 0.13f * s, 0.26f * s, 5, c);
        DrawCylinder({0, 0.0f, 0}, 0.13f * s, 0, 0.22f * s, 5, c);
        break;
    }
    rlPopMatrix();
}

void Renderer::drawGround() {
    int W = state_.width(), H = state_.height();
    float cx = (W - 1) * 0.5f * kTileSize, cz = (H - 1) * 0.5f * kTileSize;
    DrawPlane({cx, -0.7f, cz}, {4000, 4000}, Color{34, 52, 72, 255});
    DrawCube({cx, -0.22f, cz}, W * kTileSize + 0.3f, 0.3f, H * kTileSize + 0.3f, Color{78, 58, 44, 255});

    rlDisableBackfaceCulling();
    // Tile gaps fade out when zoomed far away, to avoid a moire pattern.
    const float h = kTileSize * 0.5f - 0.045f * Clamp((140.0f - cameraDist_) / 50.0f, 0.0f, 1.0f);
    for (int y = 0; y < H; y++) {
        for (int x = 0; x < W; x++) {
            Color c = ((x + y) & 1) ? Color{88, 154, 92, 255} : Color{78, 142, 84, 255};
            int j = static_cast<int>(hashf(static_cast<unsigned>(x) * 73856093u ^ static_cast<unsigned>(y) * 19349663u) * 10.0f);
            c.g = static_cast<unsigned char>(c.g + j);
            c.r = static_cast<unsigned char>(c.r + j / 2);
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
    float t = static_cast<float>(GetTime());
    float reach = std::max(30.0f, cameraDist_ * 1.1f);
    int x0 = static_cast<int>(std::floor((cameraTarget_.x - reach) / kTileSize));
    int x1 = static_cast<int>(std::ceil((cameraTarget_.x + reach) / kTileSize));
    int y0 = static_cast<int>(std::floor((cameraTarget_.z - reach) / kTileSize));
    int y1 = static_cast<int>(std::ceil((cameraTarget_.z + reach) / kTileSize));
    x0 = std::max(x0, 0); y0 = std::max(y0, 0);
    x1 = std::min(x1, state_.width() - 1); y1 = std::min(y1, state_.height() - 1);

    rlDisableBackfaceCulling();
    for (int y = y0; y <= y1; y++) {
        for (int x = x0; x <= x1; x++) {
            const Tile* tile = state_.tileAt(x, y);
            if (!tile) continue;
            Vector3 c = tileCenter(static_cast<float>(x), static_cast<float>(y));
            float dist = Vector3Distance(c, camera_.position);
            if (dist > 150.0f) continue;
            bool detail = dist < 42.0f;
            for (int r = 0; r < kResourceCount; r++) {
                int q = tile->resources[r];
                if (q <= 0) continue;
                float a = r * (2 * kPi / kResourceCount) + 0.4f;
                Vector3 sp = {c.x + std::cos(a) * 0.62f, 0.0f, c.z + std::sin(a) * 0.62f};
                if (!detail) {
                    if (dist < 85.0f) {
                        DrawCube({sp.x, 0.14f, sp.z}, 0.28f, 0.28f, 0.28f, kResColor[r]);
                    } else {
                        rlCheckRenderBatchLimit(4);
                        rlBegin(RL_QUADS);
                        rlColor4ub(kResColor[r].r, kResColor[r].g, kResColor[r].b, 255);
                        rlVertex3f(sp.x - 0.16f, 0.04f, sp.z - 0.16f); rlVertex3f(sp.x - 0.16f, 0.04f, sp.z + 0.16f);
                        rlVertex3f(sp.x + 0.16f, 0.04f, sp.z + 0.16f); rlVertex3f(sp.x + 0.16f, 0.04f, sp.z - 0.16f);
                        rlEnd();
                    }
                    continue;
                }
                int cnt = q > 5 ? 5 : q;
                float sc = q > 5 ? 1.2f : 1.0f;
                for (int k = 0; k < cnt; k++) {
                    unsigned sd = static_cast<unsigned>(x * 131 + y * 71 + r * 17 + k * 29);
                    Vector3 pp = {sp.x + (hashf(sd) - 0.5f) * (cnt > 1 ? 0.42f : 0.0f), 0.0f,
                                 sp.z + (hashf(sd + 91) - 0.5f) * (cnt > 1 ? 0.42f : 0.0f)};
                    pp.y = (k >= 3) ? 0.16f : 0.0f;
                    drawResource(r, pp, t, sc, sd);
                }
            }
        }
    }
    rlEnableBackfaceCulling();
}

void Renderer::drawPlayer(const Player& p) const {
    float t = static_cast<float>(GetTime());
    Color tc = teamColor(p.team);
    float s = p.scale() * easeOutBack(p.spawnT);
    float dy = 0, spin = 0;
    if (p.dyingT > 0) {
        float u = p.dyingT / kDeathAnimTime;
        s *= u;
        dy = (1 - u) * 1.6f;
        spin = (1 - u) * 720.0f;
    }
    if (state_.isGameOver() && p.team == state_.winnerTeamIndex())
        dy += std::fabs(std::sin(t * 6.0f + p.id)) * 0.5f;
    if (p.incanting) dy += 0.22f + 0.08f * std::sin(t * 4.0f + p.id);
    float bob = p.moving ? std::fabs(std::sin(p.animPhase)) * 0.09f : 0.02f * std::sin(t * 2.0f + p.id);
    float swing = p.moving ? std::sin(p.animPhase) * 38.0f : 0.0f;
    float squash = p.layT > 0 ? 1.0f - 0.22f * std::sin(p.layT * kPi) : 1.0f;
    float wob = p.shakeT > 0 ? std::sin(p.shakeT * 40.0f) * 25.0f : 0.0f;
    float talk = p.broadcastT > 0 ? 0.05f * std::sin(p.broadcastT * 30.0f) : 0.0f;
    Vector3 pos = playerWorldPos(p);

    if (p.dyingT <= 0)
        DrawCylinder({pos.x, 0.025f, pos.z}, 0.42f * s, 0.42f * s, 0.005f, 12, Color{0, 0, 0, 70});

    Color dark = ColorBrightness(tc, -0.45f), light = ColorBrightness(tc, 0.45f);
    Color skin = {246, 217, 178, 255};

    rlPushMatrix();
    rlTranslatef(pos.x, dy + bob, pos.z);
    rlRotatef(-p.yaw + spin + wob, 0, 1, 0);
    rlScalef(s, s * squash, s);
    for (int side = -1; side <= 1; side += 2) {
        rlPushMatrix();
        rlTranslatef(side * 0.14f, 0.30f, 0);
        rlRotatef(swing * side, 1, 0, 0);
        DrawCube({0, -0.15f, 0}, 0.14f, 0.30f, 0.16f, dark);
        rlPopMatrix();
    }
    DrawCylinder({0, 0.28f, 0}, 0.28f, 0.34f, 0.62f, 10, tc);
    DrawCylinder({0, 0.50f, 0}, 0.325f, 0.325f, 0.07f, 10, light);
    for (int side = -1; side <= 1; side += 2) {
        rlPushMatrix();
        rlTranslatef(side * 0.37f, 0.85f, 0);
        rlRotatef(-swing * side, 1, 0, 0);
        DrawCube({0, -0.2f, 0}, 0.10f, 0.40f, 0.12f, light);
        rlPopMatrix();
    }
    DrawSphereEx({0, 1.08f, 0}, 0.27f, 8, 10, skin);
    for (int side = -1; side <= 1; side += 2)
        DrawCube({side * 0.10f, 1.12f, -0.245f}, 0.07f, 0.09f + talk, 0.05f, BLACK);
    DrawCube({0, 1.03f, -0.27f}, 0.05f, 0.04f + talk * 2, 0.05f, Color{200, 90, 90, 255});
    DrawCylinder({0, 1.30f, 0}, 0.0f, 0.21f, 0.26f, 8, tc);
    rlPopMatrix();

    for (int i = 0; i < p.level && p.dyingT <= 0; i++) {
        float a = t * 2.2f + i * (2 * kPi / p.level);
        Vector3 o = {pos.x + std::cos(a) * 0.34f * s, dy + bob + 1.72f * s, pos.z + std::sin(a) * 0.34f * s};
        DrawSphereEx(o, 0.055f * s, 4, 4, Color{255, 210, 70, 255});
    }
}

void Renderer::drawEgg(const Egg& e) const {
    float t = static_cast<float>(GetTime());
    Color tc = teamColor(e.team);
    float k = easeOutBack(e.age * 3.0f);
    if (e.state != EggState::Alive) k *= Clamp(e.dyingT / kEggAnimTime, 0, 1);
    float wob = e.hatched ? std::sin(t * 18.0f + e.id) * 9.0f : 0.0f;
    Color body = mixColor(Color{250, 245, 225, 255}, tc, 0.42f);
    if (e.hatched) body = mixColor(body, Color{255, 240, 140, 255}, 0.4f);
    if (e.state == EggState::Rotten) body = mixColor(body, Color{90, 100, 70, 255}, 0.7f);

    Vector3 c = tileCenter(static_cast<float>(e.x), static_cast<float>(e.y));
    float a = hashf(static_cast<unsigned>(e.id) * 7u + 1u) * 2 * kPi;
    float r = 0.35f + 0.45f * hashf(static_cast<unsigned>(e.id) * 13u + 5u);
    Vector3 pos = {c.x + std::cos(a) * r, 0, c.z + std::sin(a) * r};

    DrawCylinder({pos.x, 0.025f, pos.z}, 0.3f * k, 0.3f * k, 0.005f, 10, Color{0, 0, 0, 70});
    rlPushMatrix();
    rlTranslatef(pos.x, 0.42f * k, pos.z);
    rlRotatef(wob, 0, 0, 1);
    rlScalef(0.32f * k, 0.42f * k, 0.32f * k);
    DrawSphereEx({0, 0, 0}, 1.0f, 10, 10, body);
    rlPopMatrix();

    if (e.hatched && e.state == EggState::Alive) {
        Vector3 b = {pos.x, 0.5f, pos.z + 0.31f};
        DrawLine3D(b, {b.x + 0.06f, b.y + 0.08f, b.z}, Color{70, 50, 30, 255});
        DrawLine3D({b.x + 0.06f, b.y + 0.08f, b.z}, {b.x - 0.04f, b.y + 0.16f, b.z}, Color{70, 50, 30, 255});
    }
}

namespace {
void quadFlat(float cx, float cz, float h, float y, Color c) {
    rlCheckRenderBatchLimit(4);
    rlBegin(RL_QUADS);
    rlColor4ub(c.r, c.g, c.b, c.a);
    rlVertex3f(cx - h, y, cz - h); rlVertex3f(cx - h, y, cz + h);
    rlVertex3f(cx + h, y, cz + h); rlVertex3f(cx + h, y, cz - h);
    rlEnd();
}
void tileOutline(int x, int y, Color c) {
    float h = kTileSize * 0.5f - 0.02f, cx = x * kTileSize, cz = y * kTileSize;
    for (int k = 0; k < 3; k++) {
        float hh = h - k * 0.03f, yy = 0.03f + k * 0.005f;
        Vector3 a = {cx - hh, yy, cz - hh}, b = {cx + hh, yy, cz - hh};
        Vector3 d = {cx + hh, yy, cz + hh}, e = {cx - hh, yy, cz + hh};
        DrawLine3D(a, b, c); DrawLine3D(b, d, c); DrawLine3D(d, e, c); DrawLine3D(e, a, c);
    }
}
} // namespace

void Renderer::drawEffects() {
    float t = static_cast<float>(GetTime());
    rlDisableBackfaceCulling();
    BeginBlendMode(BLEND_ADDITIVE);

    if (hoverTile_)
        quadFlat(hoverTileX_ * kTileSize, hoverTileY_ * kTileSize, kTileSize * 0.5f - 0.045f, 0.02f, Color{70, 110, 255, 60});
    if (selectedTile_) {
        quadFlat(selectedTileX_ * kTileSize, selectedTileY_ * kTileSize, kTileSize * 0.5f - 0.045f, 0.022f, Color{255, 230, 90, 45});
        tileOutline(selectedTileX_, selectedTileY_, Color{255, 230, 90, 255});
    }

    const Player* sp = selectedPlayer_ >= 0 ? state_.findPlayer(selectedPlayer_) : nullptr;
    if (sp) {
        Vector3 c = playerWorldPos(*sp);
        float s = sp->scale(), pulse = 0.5f + 0.5f * std::sin(t * 5.0f);
        for (int k = 0; k < 3; k++)
            DrawCircle3D({c.x, 0.05f + k * 0.01f, c.z}, (0.55f + 0.05f * pulse + k * 0.03f) * s, {1, 0, 0}, 90, Color{255, 255, 120, 255});
        DrawCylinder({c.x, (1.95f + 0.12f * std::sin(t * 4.0f)) * s, c.z}, 0.15f, 0.0f, 0.32f * s, 8, Color{255, 240, 90, 200});
    } else if (hoverPlayer_ >= 0) {
        if (const Player* hp = state_.findPlayer(hoverPlayer_)) {
            Vector3 c = playerWorldPos(*hp);
            DrawCircle3D({c.x, 0.05f, c.z}, 0.6f * hp->scale(), {1, 0, 0}, 90, Color{120, 200, 255, 255});
        }
    }

    int i = 0;
    for (const auto& inc : state_.incantations()) {
        Vector3 c = tileCenter(static_cast<float>(inc.x), static_cast<float>(inc.y));
        float pulse = 0.5f + 0.5f * std::sin(t * 4.0f + i);
        float grow = Clamp(inc.elapsed * 2.0f, 0, 1);
        for (int k = 0; k < 3; k++) {
            DrawCircle3D({c.x, 0.06f + k * 0.01f, c.z}, (1.30f - k * 0.03f) * grow, {1, 0, 0}, 90, Color{255, 215, 100, 255});
            DrawCircle3D({c.x, 0.08f + k * 0.01f, c.z}, (0.85f + 0.1f * pulse - k * 0.03f) * grow, {1, 0, 0}, 90, Color{255, 255, 255, 200});
        }
        int orbs = 5 + inc.level;
        for (int k = 0; k < orbs; k++) {
            float a = t * 1.6f + k * (2 * kPi / orbs);
            DrawSphereEx({c.x + std::cos(a) * 1.15f, 0.15f + 0.12f * std::sin(t * 3.0f + k), c.z + std::sin(a) * 1.15f},
                         0.07f, 4, 4, Color{255, 230, 130, 255});
        }
        DrawCylinderEx({c.x, 0, c.z}, {c.x, 10.0f * grow, c.z}, 0.85f, 0.35f, 14, Color{255, 225, 130, static_cast<unsigned char>(28 + 22 * pulse)});
        DrawCylinderEx({c.x, 0, c.z}, {c.x, 10.0f * grow, c.z}, 0.40f, 0.15f, 10, Color{255, 255, 255, 60});
        i++;
    }

    for (const auto& r : rings_) {
        float u = r.t / r.duration;
        float rad = r.r0 + (r.r1 - r.r0) * (1.0f - (1.0f - u) * (1.0f - u));
        Color c = r.color;
        c.a = static_cast<unsigned char>(255 * (1.0f - u));
        for (int k = 0; k < 2; k++)
            DrawCircle3D({r.pos.x, 0.07f + k * 0.01f, r.pos.z}, rad - k * 0.04f, {1, 0, 0}, 90, c);
    }
    for (const auto& p : particles_)
        DrawCube(p.pos, p.size, p.size, p.size, Fade(p.color, Clamp(p.life / p.maxLife, 0, 1)));

    EndBlendMode();
    rlEnableBackfaceCulling();
}

void Renderer::drawWorld() {
    rlSetClipPlanes(0.1, 3000.0);
    BeginMode3D(camera_);
    if (useShader_) {
        SetShaderValue(shader_, locCamPos_, &camera_.position, SHADER_UNIFORM_VEC3);
        BeginShaderMode(shader_);
    }
    if (state_.hasMap()) {
        drawGround();
        drawResources();
        for (const auto& e : state_.eggs()) drawEgg(e);
        for (const auto& p : state_.players()) drawPlayer(p);
    }
    if (useShader_) EndShaderMode();
    if (state_.hasMap()) drawEffects();
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

void Renderer::drawMinimap() {
    if (!showMinimap_ || !state_.hasMap()) return;
    Rectangle r = minimapRect(state_);
    float cell = r.width / state_.width();

    Rectangle border = {r.x - 4, r.y - 4, r.width + 8, r.height + 8};
    DrawRectangleRec(border, Color{12, 16, 28, 205});
    DrawRectangleLinesEx(border, 1, Color{90, 110, 150, 210});
    uiRects_.push_back(border);

    DrawRectangleRec(r, Color{40, 82, 50, 255});
    for (const auto& inc : state_.incantations())
        DrawRectangle(static_cast<int>(r.x + inc.x * cell - 2), static_cast<int>(r.y + inc.y * cell - 2),
                      static_cast<int>(cell) + 4, static_cast<int>(cell) + 4,
                      (static_cast<int>(GetTime() * 4) & 1) ? Color{255, 230, 100, 255} : Color{255, 140, 40, 255});
    for (const auto& e : state_.eggs()) {
        float cx = r.x + (e.x + 0.5f) * cell, cy = r.y + (e.y + 0.5f) * cell;
        DrawCircle(static_cast<int>(cx), static_cast<int>(cy), std::max(1.5f, cell * 0.25f), Color{255, 250, 230, 255});
    }
    for (const auto& p : state_.players()) {
        float ps = std::max(3.0f, cell * 0.6f);
        float wx = std::fmod(p.fx, static_cast<float>(state_.width()));
        if (wx < 0) wx += state_.width();
        float wz = std::fmod(p.fz, static_cast<float>(state_.height()));
        if (wz < 0) wz += state_.height();
        DrawRectangle(static_cast<int>(r.x + (wx + 0.5f) * cell - ps / 2), static_cast<int>(r.y + (wz + 0.5f) * cell - ps / 2),
                     static_cast<int>(ps), static_cast<int>(ps), teamColor(p.team));
    }
    if (selectedTile_)
        DrawRectangleLinesEx({r.x + selectedTileX_ * cell, r.y + selectedTileY_ * cell, cell, cell}, 1, YELLOW);

    float tx = r.x + (cameraTarget_.x / kTileSize + 0.5f) * cell, ty = r.y + (cameraTarget_.z / kTileSize + 0.5f) * cell;
    DrawLine(static_cast<int>(tx) - 5, static_cast<int>(ty), static_cast<int>(tx) + 5, static_cast<int>(ty), WHITE);
    DrawLine(static_cast<int>(tx), static_cast<int>(ty) - 5, static_cast<int>(tx), static_cast<int>(ty) + 5, WHITE);
}

void Renderer::drawHelp() {
    if (!showHelp_) return;
    int sw = GetScreenWidth(), sh = GetScreenHeight();
    Rectangle r = {sw / 2.0f - 250, sh / 2.0f - 175, 500, 350};
    DrawRectangleRec(r, Color{12, 16, 28, 205});
    DrawRectangleLinesEx(r, 1, Color{90, 110, 150, 210});
    uiRects_.push_back(r);

    int x = static_cast<int>(r.x) + 20, y = static_cast<int>(r.y) + 14;
    drawText(x, y, 20, Color{255, 210, 90, 255}, "Controls");
    y += 32;
    static const char* rows[][2] = {
        {"Left click", "select a player / a tile"},
        {"Right drag", "orbit the camera"},
        {"Middle drag", "pan the camera"},
        {"Mouse wheel", "zoom"},
        {"W A S D / arrows", "move the camera (Shift = faster)"},
        {"Q / E", "rotate the camera"},
        {"R", "reset the camera"},
        {"Tab / Shift+Tab", "cycle through players"},
        {"F", "follow the selected player"},
        {"+ / -", "change the server time unit (sst)"},
        {"M / L", "toggle minimap / player labels"},
        {"F11", "fullscreen"},
        {"Esc", "clear selection / close this help"},
        {"Minimap", "click or drag to jump around"},
        {"N", "mute / unmute background music"},
    };
    for (auto& row : rows) {
        drawText(x, y, 14, Color{255, 225, 140, 255}, "%s", row[0]);
        drawText(x + 170, y, 14, RAYWHITE, "%s", row[1]);
        y += 21;
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
        drawText(sw - 160, 10, 14, musicEnabled_ ? Color{140, 220, 170, 255} : Color{140, 150, 170, 255},
                "N: %s", musicEnabled_ ? "music on" : "muted");
    drawText(sw - 70, 10, 14, Color{170, 190, 220, 255}, "H: help");

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

    drawMinimap();
    drawHelp();
    drawOverlays();
}

void Renderer::draw() {
    drawWorld();
    drawLabels();
    drawHud();
}

} // namespace zappy
