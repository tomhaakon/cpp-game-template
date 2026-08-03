#include "game/Game.h"
#include <raylib.h>
#include <teya/core/Log.h>

namespace {
constexpr int LogicalWidth = 480;
constexpr int LogicalHeight = 320;
constexpr int WindowScale = 2;
constexpr const char *WindowTitle = "Teya Game Template";

void addBoundary(teya::collision2d::World &world,
                 teya::collision2d::Rectangle area, float thickness) {
    (void)world.add({{area.x - thickness, area.y - thickness,
                      area.width + thickness * 2.0f, thickness}, 2});
    (void)world.add({{area.x - thickness, area.y + area.height,
                      area.width + thickness * 2.0f, thickness}, 2});
    (void)world.add({{area.x - thickness, area.y, thickness, area.height}, 2});
    (void)world.add({{area.x + area.width, area.y, thickness, area.height}, 2});
}
} // namespace

Game::Game() {
    SetConfigFlags(FLAG_WINDOW_RESIZABLE);
    InitWindow(LogicalWidth * WindowScale, LogicalHeight * WindowScale, WindowTitle);
    windowOpen_ = IsWindowReady();
    SetTargetFPS(60);
    if (!canvas_.initialize(LogicalWidth, LogicalHeight)) {
        teya::core::Log::error("Graphics", "Could not create the 480x320 pixel canvas");
    }
    (void)map_.load("assets/maps/template_map.tmj");
    addBoundary(collisions_, {16.0f, 16.0f, 448.0f, 288.0f}, 8.0f);
    (void)player_.initialize(
        collisions_, {LogicalWidth * 0.5f, LogicalHeight * 0.5f});

    teya::core::Log::info("Game", "end of Game::Game.");
}

Game::~Game() {
    if (windowOpen_) {
        player_.shutdown();
        map_.unload();
        canvas_.shutdown();
        CloseWindow();
    }
}

void Game::run() {
    while (!WindowShouldClose()) {
        const float deltaTime = GetFrameTime();
        update(deltaTime);
        draw();
    }
}

void Game::update(float deltaTime) {
    player_.update(deltaTime);
}

void Game::draw() {
    canvas_.begin();
    ClearBackground(RAYWHITE);

    map_.draw();
    player_.draw();

    DrawText("WASD / ARROWS   HOLD SHIFT TO RUN", 10, 6, 10, DARKGRAY);
    canvas_.end();

    BeginDrawing();
    ClearBackground(BLACK);
    canvas_.present(GetScreenWidth(), GetScreenHeight());
    EndDrawing();
}
