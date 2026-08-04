#pragma once

#include <raylib.h>

namespace game::ui {

enum class PauseMenuAction { None, Resume, Exit };

class PauseMenu {
  public:
    [[nodiscard]] PauseMenuAction update(Vector2 pointerPosition, bool pointerPressed,
                                         int canvasWidth, int canvasHeight);
    void draw(int canvasWidth, int canvasHeight) const;

  private:
    int selectedItem_ = 0;
};

} // namespace game::ui
