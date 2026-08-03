#include "game/Game.h"
#include <raylib.h>
#include <teya/core/Log.h>

namespace {
constexpr int LogicalWidth = 480;
constexpr int LogicalHeight = 320;
constexpr int WindowScale = 2;
constexpr const char *WindowTitle = "Teya Game Template";
} // namespace

Game::Game() try {
    SetConfigFlags(FLAG_WINDOW_RESIZABLE);
    InitWindow(LogicalWidth * WindowScale, LogicalHeight * WindowScale, WindowTitle);
    windowOpen_ = IsWindowReady();
    SetTargetFPS(60);
    if (!canvas_.initialize(LogicalWidth, LogicalHeight)) {
        teya::core::Log::error("Graphics", "Could not create the 480x320 pixel canvas");
    }
    if (!world_.initialize()) {
        teya::core::Log::error("World", "Could not initialize the game world");
    }

    teya::core::Log::info("Game", "end of Game::Game.");
} catch (...) {
    // Members clean up before a constructor function-try-block handler runs.
    // The raylib window is external state, so close it explicitly as well.
    if (IsWindowReady()) CloseWindow();
    throw;
}

Game::~Game() {
    if (windowOpen_) {
        world_.shutdown();
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

void Game::update(float deltaTime) { world_.update(deltaTime); }

void Game::draw() {
    canvas_.begin();
    ClearBackground(RAYWHITE);

    world_.draw();

    canvas_.end();

    BeginDrawing();
    ClearBackground(BLACK);
    canvas_.present(GetScreenWidth(), GetScreenHeight());
    EndDrawing();
}
