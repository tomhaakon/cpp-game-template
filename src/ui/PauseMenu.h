#pragma once

#include <teya/graphics/PixelCanvas.h>

namespace game::ui {

enum class PauseMenuAction { None, Resume, Exit };

class PauseMenu {
  public:
    explicit PauseMenu(const teya::graphics::PixelCanvas &canvas);

    [[nodiscard]] PauseMenuAction update();
    void draw() const;

  private:
    const teya::graphics::PixelCanvas &canvas_;
    int selectedItem_ = 0;
};

} // namespace game::ui
