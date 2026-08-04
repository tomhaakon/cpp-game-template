#include "ui/PauseMenu.h"

#include "game/GameConfig.h"
#include "ui/UiStyle.h"

#include <raylib.h>
#include <teya/core/Input.h>

namespace game::ui {
namespace {

constexpr int ItemCount = 2;

Rectangle itemBounds(int index, int canvasWidth, int canvasHeight) {
    const int menuHeight = ItemCount * style::ButtonHeight +
                           (ItemCount - 1) * style::ButtonGap;
    return {static_cast<float>((canvasWidth - style::ButtonWidth) / 2),
            static_cast<float>((canvasHeight - menuHeight) / 2 +
                               index * (style::ButtonHeight + style::ButtonGap)),
            static_cast<float>(style::ButtonWidth),
            static_cast<float>(style::ButtonHeight)};
}

void drawCenteredText(const char *text, int centerX, int y, int fontSize, Color color) {
    DrawText(text, centerX - MeasureText(text, fontSize) / 2, y, fontSize, color);
}

} // namespace

PauseMenu::PauseMenu(const teya::graphics::PixelCanvas &canvas) : canvas_(canvas) {}

PauseMenuAction PauseMenu::update() {
    using teya::core::Action;
    using teya::core::PointerButton;
    namespace Input = teya::core::Input;

    const auto pointer = Input::pointerPosition();
    const Vector2 pointerPosition = canvas_.windowToCanvas({pointer.x, pointer.y});
    const bool pointerPressed = Input::isPressed(PointerButton::Primary);

    if (Input::isPressed(Action::Cancel)) return PauseMenuAction::Resume;
    if (Input::isPressed(Action::MoveUp)) {
        selectedItem_ = (selectedItem_ + ItemCount - 1) % ItemCount;
    }
    if (Input::isPressed(Action::MoveDown)) {
        selectedItem_ = (selectedItem_ + 1) % ItemCount;
    }
    for (int index = 0; index < ItemCount; ++index) {
        if (CheckCollisionPointRec(pointerPosition,
                                   itemBounds(index, GameConfig::CanvasWidth,
                                              GameConfig::CanvasHeight))) {
            selectedItem_ = index;
            if (pointerPressed) {
                return selectedItem_ == 0 ? PauseMenuAction::Resume
                                          : PauseMenuAction::Exit;
            }
        }
    }
    if (!Input::isPressed(Action::Confirm)) return PauseMenuAction::None;
    return selectedItem_ == 0 ? PauseMenuAction::Resume : PauseMenuAction::Exit;
}

void PauseMenu::draw() const {
    ClearBackground(style::Background);

    const int centerX = GameConfig::CanvasWidth / 2;
    drawCenteredText("GAME PAUSED", centerX, GameConfig::CanvasHeight / 4,
                     style::TitleFontSize, style::Text);

    constexpr const char *items[ItemCount] = {"RESUME", "EXIT"};
    for (int index = 0; index < ItemCount; ++index) {
        const Rectangle bounds = itemBounds(index, GameConfig::CanvasWidth,
                                            GameConfig::CanvasHeight);
        const bool selected = index == selectedItem_;
        DrawRectangleRec(bounds, selected ? style::Accent : style::Panel);
        drawCenteredText(items[index], centerX,
                         static_cast<int>(bounds.y) +
                             (style::ButtonHeight - style::ButtonFontSize) / 2,
                         style::ButtonFontSize,
                         selected ? style::Background : style::Text);
    }

    drawCenteredText("ESC TO RESUME", centerX,
                     GameConfig::CanvasHeight - style::HintFontSize - 16,
                     style::HintFontSize, style::MutedText);
}

} // namespace game::ui
