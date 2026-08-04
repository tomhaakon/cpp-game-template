#include "ui/MainMenu.h"

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

MainMenuAction MainMenu::update(Vector2 pointerPosition, bool pointerPressed,
                                int canvasWidth, int canvasHeight) {
    using teya::core::Action;
    namespace Input = teya::core::Input;

    if (Input::isPressed(Action::MoveUp)) {
        selectedItem_ = (selectedItem_ + ItemCount - 1) % ItemCount;
    }
    if (Input::isPressed(Action::MoveDown)) {
        selectedItem_ = (selectedItem_ + 1) % ItemCount;
    }
    for (int index = 0; index < ItemCount; ++index) {
        if (CheckCollisionPointRec(pointerPosition,
                                   itemBounds(index, canvasWidth, canvasHeight))) {
            selectedItem_ = index;
            if (pointerPressed) {
                return selectedItem_ == 0 ? MainMenuAction::StartGame
                                          : MainMenuAction::Quit;
            }
        }
    }
    if (Input::isPressed(Action::Cancel)) return MainMenuAction::Quit;
    if (!Input::isPressed(Action::Confirm)) return MainMenuAction::None;

    return selectedItem_ == 0 ? MainMenuAction::StartGame : MainMenuAction::Quit;
}

void MainMenu::draw(int canvasWidth, int canvasHeight) const {
    ClearBackground(style::Background);

    const int centerX = canvasWidth / 2;
    const int titleY = canvasHeight / 4;
    drawCenteredText("TEYA GAME", centerX, titleY, style::TitleFontSize, style::Text);

    constexpr const char *items[ItemCount] = {"START", "QUIT"};
    for (int index = 0; index < ItemCount; ++index) {
        const Rectangle bounds = itemBounds(index, canvasWidth, canvasHeight);
        const bool selected = index == selectedItem_;
        DrawRectangleRec(bounds, selected ? style::Accent : style::Panel);
        drawCenteredText(items[index], centerX,
                         static_cast<int>(bounds.y) +
                             (style::ButtonHeight - style::ButtonFontSize) / 2,
                         style::ButtonFontSize,
                         selected ? style::Background : style::Text);
    }

    drawCenteredText("W/S OR ARROWS - ENTER TO SELECT", centerX,
                     canvasHeight - style::HintFontSize - 16,
                     style::HintFontSize, style::MutedText);
}

} // namespace game::ui
