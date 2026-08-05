#pragma once

#include "ui/MainMenu.h"
#include "ui/PauseMenu.h"
#include "world/World.h"

#include <teya/graphics/PixelCanvas.h>
#if TEYA_ENABLE_EDITOR
#include <teya/editor/EditorHost.h>
#include <vector>
#endif

enum class GameAction { None, Exit };

class GameStateMachine {
  public:
    explicit GameStateMachine(const teya::graphics::PixelCanvas &canvas);

    [[nodiscard]] bool initialize();
    [[nodiscard]] GameAction update(float deltaTime, bool gameplayInputEnabled = true);
    void draw() const;
    [[nodiscard]] const char* stateName() const;
    [[nodiscard]] const char* mapName() const;
    [[nodiscard]] int colliderCount() const;
#if TEYA_ENABLE_EDITOR
    [[nodiscard]] std::vector<teya::editor::RuntimeProperty> editorProperties(teya::editor::RuntimeObjectId id) const;
#endif

  private:
    enum class GameState { MainMenu, Playing, PauseMenu };

    [[nodiscard]] GameAction updateMainMenu();
    [[nodiscard]] GameAction updatePlaying(float deltaTime, bool gameplayInputEnabled);
    [[nodiscard]] GameAction updatePauseMenu();

    game::World world_;
    game::ui::MainMenu mainMenu_;
    game::ui::PauseMenu pauseMenu_;
    GameState currentState_ = GameState::MainMenu;
};
