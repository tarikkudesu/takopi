#pragma once

#include <string>
#include <utility>
#include <vector>

#include "raylib.h"

#include "game/GameState.hpp"
#include "network/TcpClient.hpp"

namespace zappy {

// Owns every raylib resource (window, camera, shader) and draws GameState
// as a 3D scene plus a 2D HUD. It also owns the transient, purely visual
// effects (particles, rings, the event log) that GameState knows nothing
// about, and is the only layer allowed to call TcpClient::send() for
// interactive, on-demand requests (selection refreshes, periodic "mct").
class Renderer {
public:
    Renderer(GameState& state, TcpClient& net) : state_(state), net_(net) {}

    bool init(int width, int height, const char* title);
    void shutdown();

    bool shouldClose() const;
    void beginFrame();
    void endFrame();

    void handleInput(float dt);
    void processEvents(const std::vector<GameEvent>& events);
    void update(float dt);
    void draw();

    void takeScreenshot(const char* path);

private:
    GameState& state_;
    TcpClient& net_;

    // --- camera --------------------------------------------------------------
    Camera3D camera_{};
    Vector3 cameraTarget_{};
    float cameraYaw_ = 0, cameraPitch_ = 0.95f;
    float cameraDist_ = 30, cameraDistGoal_ = 30;
    bool follow_ = false;

    // --- shading ---------------------------------------------------------------
    Shader shader_{};
    bool useShader_ = false;
    int locCamPos_ = -1, locFogColor_ = -1;

    // --- selection / hover -------------------------------------------------------
    int selectedPlayer_ = -1;
    bool selectedTile_ = false;
    int selectedTileX_ = 0, selectedTileY_ = 0;
    float selectionRefreshTimer_ = 0;
    bool hoverTile_ = false;
    int hoverTileX_ = 0, hoverTileY_ = 0;
    int hoverPlayer_ = -1;
    bool minimapDrag_ = false;

    // --- toggles -----------------------------------------------------------------
    bool showMinimap_ = true;
    bool showLabels_ = true;
    bool showHelp_ = false;

    float mapRefreshTimer_ = 0;
    float fireworkTimer_ = 0;

    // --- audio -------------------------------------------------------------------
    // All sounds are synthesized at startup (no asset files to ship or lose, so
    // the client still "works out of the box on the dumps" as the subject
    // requires). Broadcasts, joins, deaths and incantations each get a distinct
    // tone; a soft ambient loop plays while exploring and a short original
    // fanfare plays once when the game ends.
    bool audioReady_ = false;
    bool musicEnabled_ = true;
    bool previousGameOver_ = false;
    Sound sndBroadcast_{};
    Sound sndJoin_{};
    Sound sndDeath_{};
    Sound sndIncantSuccess_{};
    Sound sndIncantFail_{};
    Sound sndVictory_{};
    Sound sndAmbientLoop_{};

    // --- effects: particles, rings, floating log ------------------------------------
    struct Particle { Vector3 pos, vel; Color color; float life, maxLife, size, gravity; };
    struct Ring { Vector3 pos; Color color; float t, duration, r0, r1; };
    struct LogEntry { std::string text; Color color; double time; };

    std::vector<Particle> particles_;
    std::vector<Ring> rings_;
    std::vector<LogEntry> log_;
    std::vector<Rectangle> uiRects_;   // panels drawn this frame that swallow clicks

    // --- small helpers -----------------------------------------------------------
    Color teamColor(int teamIndex) const;
    void resetCamera();
    void spawnBurst(Vector3 pos, Color color, int count, float speed, float up, float life, float size);
    void spawnRing(Vector3 pos, Color color, float r0, float r1, float duration);
    void log(Color color, const std::string& text);

    void selectPlayer(int id);
    void selectTile(int x, int y);
    void clearSelection();
    void changeFrequency(int direction);

    static Sound makeMelody(const std::vector<std::pair<float, float>>& notes, float volume);
    static Sound makeTone(float freqHz, float durationSec, float volume);
    void loadSounds();
    void unloadSounds();
    float broadcastVolumeFor(Vector3 worldPos) const;

    Vector3 playerWorldPos(const Player& p) const;
    Vector3 tileCenter(float x, float y) const;
    bool projectToScreen(Vector3 world, Vector2& out) const;
    bool isOverUi(Vector2 mouse) const;

    // --- 3D drawing ----------------------------------------------------------------
    void drawGround();
    void drawResources();
    void drawResource(int type, Vector3 pos, float time, float scale, unsigned seed) const;
    void drawPlayer(const Player& p) const;
    void drawEgg(const Egg& e) const;
    void drawEffects();
    void drawWorld();

    // --- 2D drawing ----------------------------------------------------------------
    void drawHud();
    void drawTeamsPanel();
    void drawSelectionPanel();
    void drawMinimap();
    void drawHelp();
    void drawOverlays();
    void drawLabels();
};

} // namespace zappy
