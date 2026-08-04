#pragma once

#include <raylib.h>

namespace game::ui {

enum class MainMenuAction { None, StartGame, Quit };

class MainMenu {
  public:
    [[nodiscard]] MainMenuAction update(Vector2 pointerPosition, bool pointerPressed,
                                        int canvasWidth, int canvasHeight);
    void draw(int canvasWidth, int canvasHeight) const;

  private:
    int selectedItem_ = 0;
};

} // namespace game::ui
