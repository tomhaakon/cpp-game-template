#include "game/Game.h"
#include "game/GameConfig.h"
#include <raylib.h>
#include <stdexcept>
#include <teya/core/Log.h>

Game::Game() : gameStates_(canvas_) {
    if (!canvas_.initialize(GameConfig::CanvasWidth, GameConfig::CanvasHeight)) {
        throw std::runtime_error("Could not initialize the pixel canvas");
    }
    if (!gameStates_.initialize()) {
        throw std::runtime_error("Could not initialize the game world");
    }

    teya::core::Log::info("Game", "end of Game::Game.");
}

void Game::run() {
    while (!exitRequested_ && !WindowShouldClose()) {
        const float deltaTime = GetFrameTime();
        update(deltaTime);
        draw();
    }
}

void Game::update(float deltaTime) {
    if (gameStates_.update(deltaTime) == GameAction::Exit) {
        exitRequested_ = true;
    }
}

void Game::draw() {
    canvas_.begin();
    gameStates_.draw();
    canvas_.end();

    BeginDrawing();
    ClearBackground(BLACK);
    canvas_.present(GetScreenWidth(), GetScreenHeight());
    EndDrawing();
}
