#pragma once

#include "game/GameWindow.h"
#include "ui/MainMenu.h"
#include "ui/PauseMenu.h"
#include "world/World.h"

#include <teya/graphics/PixelCanvas.h>

class Game {
  public:
    Game();

    Game(const Game &) = delete;            // Copy construction
    Game &operator=(const Game &) = delete; // Copy assignment

    Game(Game &&) = delete;            // Move construction
    Game &operator=(Game &&) = delete; // Move assignment

    void run();

  private:
    enum class State { MainMenu, Playing, PauseMenu };

    void update(float deltaTime);
    void draw();

    GameWindow window_;
    teya::graphics::PixelCanvas canvas_;
    game::World world_;
    game::ui::MainMenu mainMenu_;
    game::ui::PauseMenu pauseMenu_;
    State state_ = State::MainMenu;
    bool exitRequested_ = false;
};
