#include "game/Game.h"
#include <raylib.h>
#include <teya/core/Log.h>

namespace {
constexpr int LogicalWidth = 480;
constexpr int LogicalHeight = 320;
constexpr int WindowScale = 2;
constexpr const char *WindowTitle = "Teya Game Template";
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
    teya::core::Log::info("Game", "end of Game::Game.");
}

Game::~Game() {
    if (windowOpen_) {
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

void Game::update(float deltaTime) { (void)deltaTime; }

void Game::draw() {
    canvas_.begin();
    ClearBackground(RAYWHITE);

    map_.draw();
    canvas_.end();

    BeginDrawing();
    ClearBackground(BLACK);
    canvas_.present(GetScreenWidth(), GetScreenHeight());
    EndDrawing();
}
