#pragma once

#include "world/World.h"

#include <teya/graphics/PixelCanvas.h>

class Game {
  public:
    Game();
    ~Game();

    Game(const Game &) = delete;
    Game &operator=(const Game &) = delete;
    Game(Game &&) = delete;
    Game &operator=(Game &&) = delete;

    void run();

  private:
    void update(float deltaTime);
    void draw();

    bool windowOpen_ = false;
    teya::graphics::PixelCanvas canvas_;
    game::World world_;
};
