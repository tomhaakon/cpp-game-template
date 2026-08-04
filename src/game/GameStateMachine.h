#pragma once

#include "ui/MainMenu.h"
#include "ui/PauseMenu.h"
#include "world/World.h"

#include <teya/graphics/PixelCanvas.h>

enum class GameAction { None, Exit };

class GameStateMachine {
  public:
    explicit GameStateMachine(const teya::graphics::PixelCanvas &canvas);

    [[nodiscard]] bool initialize();
    [[nodiscard]] GameAction update(float deltaTime);
    void draw() const;

  private:
    enum class GameState { MainMenu, Playing, PauseMenu };

    [[nodiscard]] GameAction updateMainMenu();
    [[nodiscard]] GameAction updatePlaying(float deltaTime);
    [[nodiscard]] GameAction updatePauseMenu();

    game::World world_;
    game::ui::MainMenu mainMenu_;
    game::ui::PauseMenu pauseMenu_;
    GameState currentState_ = GameState::MainMenu;
};
