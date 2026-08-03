#pragma once

#include <string>
#include <teya/tiled/MapRenderer.h>
#include <teya/graphics/PixelCanvas.h>

class Game {
  public:
    Game(int width, int height, std::string title);
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
};
