#include "game/Game.h"
#include "game/GameConfig.h"
#include <raylib.h>
#include <stdexcept>
#include <teya/core/Log.h>
#include <teya/core/Profile.h>

Game::Game() : gameStates_(canvas_) {
    TEYA_PROFILE_ZONE_NAMED("Game::Game");
    if (!canvas_.initialize(GameConfig::CanvasWidth, GameConfig::CanvasHeight)) {
        throw std::runtime_error("Could not initialize the pixel canvas");
    }
    if (!gameStates_.initialize()) {
        throw std::runtime_error("Could not initialize the game world");
    }

    teya::core::Log::info("Game", "end of Game::Game.");
}

void Game::run() {
    TEYA_PROFILE_THREAD("Main");
    while (!exitRequested_ && !WindowShouldClose()) {
        TEYA_PROFILE_ZONE_NAMED("Game frame");
        const float deltaTime = GetFrameTime();
        TEYA_PROFILE_PLOT("Frame time (ms)", deltaTime * 1000.0f);
        update(deltaTime);
        draw();
        TEYA_PROFILE_FRAME();
    }
}

void Game::update(float deltaTime) {
    TEYA_PROFILE_ZONE_NAMED("Game::update");
    if (gameStates_.update(deltaTime) == GameAction::Exit) {
        exitRequested_ = true;
    }
}

void Game::draw() {
    TEYA_PROFILE_ZONE_NAMED("Game::draw");
    canvas_.begin();
    gameStates_.draw();
    canvas_.end();

    BeginDrawing();
    ClearBackground(BLACK);
    canvas_.present(GetScreenWidth(), GetScreenHeight());
    EndDrawing();
}
