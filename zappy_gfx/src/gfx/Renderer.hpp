#pragma once

#include <string>
#include <vector>

#include "raylib.h"

#include "game/GameState.hpp"
#include "network/TcpClient.hpp"

namespace zappy {

// Coordinates the camera, world drawing, selection, audio, and HUD.
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

private:
    GameState& state_;
    TcpClient& net_;

    // --- camera --------------------------------------------------------------
    Camera3D camera_{};
    Vector3 cameraTarget_{};
    float cameraYaw_ = 0, cameraPitch_ = 0.95f;
    float cameraDist_ = 30, cameraDistGoal_ = 30;
    bool follow_ = false;

    // --- selection / hover -------------------------------------------------------
    int selectedPlayer_ = -1;
    bool selectedTile_ = false;
    int selectedTileX_ = 0, selectedTileY_ = 0;
    float selectionRefreshTimer_ = 0;
    bool hoverTile_ = false;
    int hoverTileX_ = 0, hoverTileY_ = 0;
    int hoverPlayer_ = -1;
    bool showLabels_ = true;

    float mapRefreshTimer_ = 0;

    bool audioReady_ = false;
    Sound sndBroadcast_{};

    struct LogEntry { std::string text; Color color; double time; };

    std::vector<LogEntry> log_;
    std::vector<Rectangle> uiRects_;   // panels drawn this frame that swallow clicks

    // --- small helpers -----------------------------------------------------------
    Color teamColor(int teamIndex) const;
    void resetCamera();
    void log(Color color, const std::string& text);

    void selectPlayer(int id);
    void selectTile(int x, int y);
    void clearSelection();
    void changeFrequency(int direction);

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
    void drawResource(int type, Vector3 pos, float scale) const;
    void drawPlayer(const Player& p) const;
    void drawEgg(const Egg& e) const;
    void drawWorld();

    // --- 2D drawing ----------------------------------------------------------------
    void drawHud();
    void drawTeamsPanel();
    void drawSelectionPanel();
    void drawOverlays();
    void drawLabels();
};

} // namespace zappy
