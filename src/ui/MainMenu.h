#pragma once

#include <teya/graphics/PixelCanvas.h>
#include <optional>

namespace game::ui {

enum class MainMenuAction { None, StartGame, Quit };

class MainMenu {
  public:
    explicit MainMenu(const teya::graphics::PixelCanvas &canvas);

    [[nodiscard]] MainMenuAction update(std::optional<Vector2> canvasPointer = std::nullopt);
    void draw() const;

  private:
    const teya::graphics::PixelCanvas &canvas_;
    int selectedItem_ = 0;
};

} // namespace game::ui
