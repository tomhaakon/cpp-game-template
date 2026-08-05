#include "game/GameStateMachine.h"

#include <raylib.h>
#include <teya/core/Input.h>
#include <teya/core/Profile.h>

GameStateMachine::GameStateMachine(const teya::graphics::PixelCanvas &canvas)
    : mainMenu_(canvas), pauseMenu_(canvas) {}

bool GameStateMachine::initialize() {
    TEYA_PROFILE_ZONE_NAMED("GameStateMachine::initialize");
    currentState_ = GameState::MainMenu;
    return world_.initialize();
}

bool GameStateMachine::restart() {
    TEYA_PROFILE_ZONE_NAMED("GameStateMachine::restart");
    world_.shutdown();
    currentState_ = GameState::MainMenu;
    return world_.initialize();
}

GameAction GameStateMachine::update(float deltaTime, bool gameplayInputEnabled,
                                    std::optional<Vector2> canvasPointer) {
    TEYA_PROFILE_ZONE_NAMED("GameStateMachine::update");
    switch (currentState_) {
    case GameState::MainMenu:
        return updateMainMenu(gameplayInputEnabled, canvasPointer);
    case GameState::Playing:
        return updatePlaying(deltaTime, gameplayInputEnabled);
    case GameState::PauseMenu:
        return updatePauseMenu(gameplayInputEnabled, canvasPointer);
    }
    return GameAction::None;
}

GameAction GameStateMachine::updateMainMenu(bool gameplayInputEnabled,
                                            std::optional<Vector2> canvasPointer) {
    if (!gameplayInputEnabled) return GameAction::None;
    switch (mainMenu_.update(canvasPointer)) {
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

GameAction GameStateMachine::updatePlaying(float deltaTime, bool gameplayInputEnabled) {
    if (gameplayInputEnabled && teya::core::Input::isPressed(teya::core::Action::Cancel)) {
        currentState_ = GameState::PauseMenu;
    } else {
        world_.update(deltaTime, gameplayInputEnabled);
    }
    return GameAction::None;
}

const char* GameStateMachine::stateName() const { switch(currentState_){case GameState::MainMenu:return "Main Menu";case GameState::Playing:return "Playing";case GameState::PauseMenu:return "Pause Menu";}return "Unknown"; }
const char* GameStateMachine::mapName() const{return world_.mapName();}
int GameStateMachine::colliderCount() const{return world_.colliderCount();}
#if TEYA_ENABLE_EDITOR
std::vector<teya::editor::RuntimeProperty> GameStateMachine::editorProperties(teya::editor::RuntimeObjectId id) const { if(id==5)return world_.playerProperties();if(id==4)return {{"Map",world_.mapName()}};if(id==6)return {{"Collider count",std::to_string(world_.colliderCount())}};return {{"Name",id==1?"Game":id==3?"World":stateName()}}; }
void GameStateMachine::drawDebug(const teya::editor::EditorDebugDrawSettings &settings) const {
    if (currentState_ == GameState::Playing)
        world_.drawDebug(settings);
}
#endif

GameAction GameStateMachine::updatePauseMenu(bool gameplayInputEnabled,
                                             std::optional<Vector2> canvasPointer) {
    if (!gameplayInputEnabled) return GameAction::None;
    switch (pauseMenu_.update(canvasPointer)) {
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
    TEYA_PROFILE_ZONE_NAMED("GameStateMachine::draw");
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
