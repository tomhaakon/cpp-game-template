#include "game/GameStateMachine.h"

#include <raylib.h>
#include <teya/core/Input.h>

GameStateMachine::GameStateMachine(const teya::graphics::PixelCanvas &canvas)
    : mainMenu_(canvas), pauseMenu_(canvas) {}

bool GameStateMachine::initialize() { return world_.initialize(); }

GameAction GameStateMachine::update(float deltaTime) {
    switch (currentState_) {
    case GameState::MainMenu:
        return updateMainMenu();
    case GameState::Playing:
        return updatePlaying(deltaTime);
    case GameState::PauseMenu:
        return updatePauseMenu();
    }
    return GameAction::None;
}

GameAction GameStateMachine::updateMainMenu() {
    switch (mainMenu_.update()) {
    case game::ui::MainMenuAction::StartGame:
        currentState_ = GameState::Playing;
        break;
    case game::ui::MainMenuAction::Quit:
        return GameAction::Exit;
    case game::ui::MainMenuAction::None:
        break;
    }
    return GameAction::None;
}

GameAction GameStateMachine::updatePlaying(float deltaTime) {
    if (teya::core::Input::isPressed(teya::core::Action::Cancel)) {
        currentState_ = GameState::PauseMenu;
    } else {
        world_.update(deltaTime);
    }
    return GameAction::None;
}

GameAction GameStateMachine::updatePauseMenu() {
    switch (pauseMenu_.update()) {
    case game::ui::PauseMenuAction::Resume:
        currentState_ = GameState::Playing;
        break;
    case game::ui::PauseMenuAction::Exit:
        return GameAction::Exit;
    case game::ui::PauseMenuAction::None:
        break;
    }
    return GameAction::None;
}

void GameStateMachine::draw() const {
    switch (currentState_) {
    case GameState::MainMenu:
        mainMenu_.draw();
        break;
    case GameState::Playing:
        ClearBackground(RAYWHITE);
        world_.draw();
        break;
    case GameState::PauseMenu:
        pauseMenu_.draw();
        break;
    }
}
