#include "game/Game.h"
#include "game/GameConfig.h"
#include <raylib.h>
#include <teya/core/Log.h>

Game::Game() {
    if (!window_.isOpen()) {
        teya::core::Log::error("Graphics", "Could not create the game window");
        return;
    }

    if (!canvas_.initialize(GameConfig::CanvasWidth, GameConfig::CanvasHeight)) {
        teya::core::Log::error("Graphics", "Could not create the 480x320 pixel canvas");
    }
    if (!world_.initialize()) {
        teya::core::Log::error("World", "Could not initialize the game world");
    }

    teya::core::Log::info("Game", "end of Game::Game.");
}

Game::~Game() {
    if (window_.isOpen()) {
        world_.shutdown();
        canvas_.shutdown();
    }
}

void Game::run() {
    if (!window_.isOpen()) return;
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
