#pragma once

#include "game/GameWindow.h"
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
    void update(float deltaTime);
    void draw();

    GameWindow window_;
    teya::graphics::PixelCanvas canvas_;
    game::World world_;
};
