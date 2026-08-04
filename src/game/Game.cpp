#include "game/Game.h"
#include "game/GameConfig.h"
#include <raylib.h>
#include <stdexcept>
#include <teya/core/Input.h>
#include <teya/core/Log.h>

Game::Game() {
    if (!canvas_.initialize(GameConfig::CanvasWidth, GameConfig::CanvasHeight)) {
        throw std::runtime_error("Could not initialize the pixel canvas");
    }
    if (!world_.initialize()) {
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
    if (state_ == State::Playing) {
        if (teya::core::Input::isPressed(teya::core::Action::Cancel)) {
            state_ = State::PauseMenu;
            return;
        }
        world_.update(deltaTime);
        return;
    }

    const Vector2 pointerPosition = canvas_.windowToCanvas(
        GetMousePosition(), GetScreenWidth(), GetScreenHeight());
    const bool pointerPressed = IsMouseButtonPressed(MOUSE_BUTTON_LEFT);

    if (state_ == State::PauseMenu) {
        switch (pauseMenu_.update(pointerPosition, pointerPressed,
                                  GameConfig::CanvasWidth, GameConfig::CanvasHeight)) {
        case game::ui::PauseMenuAction::Resume:
            state_ = State::Playing;
            break;
        case game::ui::PauseMenuAction::Exit:
            exitRequested_ = true;
            break;
        case game::ui::PauseMenuAction::None:
            break;
        }
        return;
    }

    switch (mainMenu_.update(pointerPosition, pointerPressed,
                             GameConfig::CanvasWidth, GameConfig::CanvasHeight)) {
    case game::ui::MainMenuAction::StartGame:
        state_ = State::Playing;
        break;
    case game::ui::MainMenuAction::Quit:
        exitRequested_ = true;
        break;
    case game::ui::MainMenuAction::None:
        break;
    }
}

void Game::draw() {
    canvas_.begin();
    if (state_ == State::Playing) {
        ClearBackground(RAYWHITE);
        world_.draw();
    } else if (state_ == State::PauseMenu) {
        pauseMenu_.draw(GameConfig::CanvasWidth, GameConfig::CanvasHeight);
    } else {
        mainMenu_.draw(GameConfig::CanvasWidth, GameConfig::CanvasHeight);
    }

    canvas_.end();

    BeginDrawing();
    ClearBackground(BLACK);
    canvas_.present(GetScreenWidth(), GetScreenHeight());
    EndDrawing();
}
