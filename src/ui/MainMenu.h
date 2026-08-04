#pragma once

#include <teya/graphics/PixelCanvas.h>

namespace game::ui {

enum class MainMenuAction { None, StartGame, Quit };

class MainMenu {
  public:
    explicit MainMenu(const teya::graphics::PixelCanvas &canvas);

    [[nodiscard]] MainMenuAction update();
    void draw() const;

  private:
    const teya::graphics::PixelCanvas &canvas_;
    int selectedItem_ = 0;
};

} // namespace game::ui
