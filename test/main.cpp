#include "raylib.h"

#include <cmath>
#include <cstdio>

// Camera settings (angles are in radians).
Vector3 cameraTarget_ = { 0.0f, 1.0f, 0.0f };
float cameraDist_ = 12.0f;
float cameraYaw_ = 0.785398f;   // 45 degrees
float cameraPitch_ = 0.523599f; // 30 degrees

// Keep the camera at a useful angle and distance.
float ClampFloat(float value, float minValue, float maxValue)
{
    if (value < minValue) return minValue;
    if (value > maxValue) return maxValue;
    return value;
}

// Calculate the camera's actual position from the four settings.
Vector3 CalculateCameraPosition()
{
    Vector3 position;

    position.x = cameraTarget_.x
        + cameraDist_ * cosf(cameraPitch_) * sinf(cameraYaw_);

    position.y = cameraTarget_.y
        + cameraDist_ * sinf(cameraPitch_);

    position.z = cameraTarget_.z
        + cameraDist_ * cosf(cameraPitch_) * cosf(cameraYaw_);

    return position;
}

void ResetCameraSettings()
{
    cameraTarget_ = { 0.0f, 1.0f, 0.0f };
    cameraDist_ = 12.0f;
    cameraYaw_ = 0.785398f;
    cameraPitch_ = 0.523599f;
}

void UpdateCameraSettings()
{
    const float angleSpeed = 1.0f * GetFrameTime();
    const float moveSpeed = 5.0f * GetFrameTime();
    const float zoomSpeed = 8.0f * GetFrameTime();

    // A / D: orbit horizontally around the target.
    if (IsKeyDown(KEY_A)) cameraYaw_ -= angleSpeed;
    if (IsKeyDown(KEY_D)) cameraYaw_ += angleSpeed;

    // W / S: change the vertical viewing angle.
    if (IsKeyDown(KEY_W)) cameraPitch_ += angleSpeed;
    if (IsKeyDown(KEY_S)) cameraPitch_ -= angleSpeed;

    // Keep pitch away from directly vertical angles.
    cameraPitch_ = ClampFloat(cameraPitch_, 0.10f, 1.45f);

    // Plus / minus: change distance from the target.
    if (IsKeyDown(KEY_EQUAL) || IsKeyDown(KEY_KP_ADD))
        cameraDist_ -= zoomSpeed;

    if (IsKeyDown(KEY_MINUS) || IsKeyDown(KEY_KP_SUBTRACT))
        cameraDist_ += zoomSpeed;

    cameraDist_ = ClampFloat(cameraDist_, 3.0f, 35.0f);

    // Arrow keys: move the point the camera looks at.
    if (IsKeyDown(KEY_LEFT))  cameraTarget_.x -= moveSpeed;
    if (IsKeyDown(KEY_RIGHT)) cameraTarget_.x += moveSpeed;
    if (IsKeyDown(KEY_UP))    cameraTarget_.z -= moveSpeed;
    if (IsKeyDown(KEY_DOWN))  cameraTarget_.z += moveSpeed;

    if (IsKeyPressed(KEY_R))
        ResetCameraSettings();
}

void DrawWorld(Vector3 cameraPosition)
{
    // Ground reference grid (on the XZ plane).
    DrawGrid(20, 1.0f);

    // World axes: X = red, Y = green, Z = blue.
    DrawLine3D({ -10.0f, 0.02f, 0.0f }, { 10.0f, 0.02f, 0.0f }, RED);
    DrawLine3D({ 0.0f, 0.02f, -10.0f }, { 0.0f, 0.02f, 10.0f }, BLUE);
    DrawLine3D({ 0.0f, 0.0f, 0.0f }, { 0.0f, 4.0f, 0.0f }, GREEN);

    // The cube stays at the world origin. Its bottom sits on the ground.
    DrawCube({ 0.0f, 1.0f, 0.0f }, 2.0f, 2.0f, 2.0f, SKYBLUE);
    DrawCubeWires({ 0.0f, 1.0f, 0.0f }, 2.0f, 2.0f, 2.0f, DARKBLUE);

    // Yellow = the point the camera looks at.
    DrawSphere(cameraTarget_, 0.22f, YELLOW);

    // The camera is located at cameraPosition. It is the viewer, so a 3D
    // marker placed exactly there cannot normally be seen by that camera.
    // Its coordinates are therefore shown in the 2D HUD below.

    // A line illustrates the direction from the camera toward its target.
    DrawLine3D(cameraPosition, cameraTarget_, ORANGE);

    // Small labels in the 3D world.
    DrawSphere({ 0.0f, 0.0f, 0.0f }, 0.08f, WHITE);
}

void DrawHelp(Vector3 cameraPosition)
{
    DrawRectangle(10, 10, 390, 205, Fade(BLACK, 0.78f));

    DrawText("RAYLIB CAMERA LAB", 22, 20, 20, RAYWHITE);
    DrawText("BLUE cube = fixed world object", 22, 50, 15, SKYBLUE);
    DrawText("YELLOW = camera target", 22, 72, 15, YELLOW);
    DrawText("Camera position is shown in the HUD below", 22, 94, 15, RED);
    DrawText("ORANGE line = camera looks this way", 22, 116, 15, ORANGE);

    char line[160];

    std::snprintf(line, sizeof(line),
        "Target: X %.1f   Y %.1f   Z %.1f",
        cameraTarget_.x, cameraTarget_.y, cameraTarget_.z);
    DrawText(line, 22, 145, 15, RAYWHITE);

    std::snprintf(line, sizeof(line),
        "Distance: %.1f   Yaw: %.2f rad   Pitch: %.2f rad",
        cameraDist_, cameraYaw_, cameraPitch_);
    DrawText(line, 22, 168, 15, RAYWHITE);

    std::snprintf(line, sizeof(line),
        "Camera: X %.1f   Y %.1f   Z %.1f",
        cameraPosition.x, cameraPosition.y, cameraPosition.z);
    DrawText(line, 22, 190, 15, RED);

    DrawRectangle(10, GetScreenHeight() - 94, 590, 84, Fade(BLACK, 0.78f));
    DrawText("A/D: orbit    W/S: pitch    +/-: zoom", 22,
             GetScreenHeight() - 82, 17, RAYWHITE);
    DrawText("Arrow keys: move target    R: reset    ESC: quit", 22,
             GetScreenHeight() - 54, 17, RAYWHITE);

    (void)cameraPosition;
}

int main()
{
    InitWindow(1100, 750, "Raylib - 3D Camera Laboratory");
    SetTargetFPS(60);

    while (!WindowShouldClose())
    {
        UpdateCameraSettings();

        // Recalculate camera position every frame from the settings.
        Vector3 cameraPosition = CalculateCameraPosition();

        Camera3D camera{};
        camera.position = cameraPosition;
        camera.target = cameraTarget_;
        camera.up = { 0.0f, 1.0f, 0.0f };
        camera.fovy = 25.0f;
        camera.projection = CAMERA_PERSPECTIVE;

        BeginDrawing();
        ClearBackground(RAYWHITE);

        BeginMode3D(camera);
        DrawWorld(cameraPosition);
        EndMode3D();

        // This text is 2D UI, so draw it after EndMode3D().
        DrawHelp(cameraPosition);

        EndDrawing();
    }

    CloseWindow();
    return 0;
}
