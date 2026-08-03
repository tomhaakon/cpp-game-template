#pragma once

#include "player/Player.h"

#include <teya/collision2d/World.h>
#include <teya/graphics/PixelCanvas.h>
#include <teya/tiled/MapRenderer.h>

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
    teya::tiled::MapRenderer map_;
    teya::collision2d::World collisions_;
    Player player_;
};
