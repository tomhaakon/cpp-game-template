#pragma once

class GameWindow {
  public:
    GameWindow();
    ~GameWindow();

    GameWindow(const GameWindow &) = delete;
    GameWindow &operator=(const GameWindow &) = delete;
    GameWindow(GameWindow &&) = delete;
    GameWindow &operator=(GameWindow &&) = delete;

};
