#include "game/GameWindow.h"

#include "game/GameConfig.h"

#include <raylib.h>
#include <stdexcept>

GameWindow::GameWindow() {
    SetConfigFlags(FLAG_WINDOW_RESIZABLE);
    InitWindow(GameConfig::CanvasWidth * GameConfig::InitialWindowScale,
               GameConfig::CanvasHeight * GameConfig::InitialWindowScale,
               GameConfig::WindowTitle);
    if (!IsWindowReady()) {
        throw std::runtime_error("Could not create the game window");
    }
    SetExitKey(KEY_NULL);
    SetTargetFPS(GameConfig::TargetFps);
}

GameWindow::~GameWindow() { CloseWindow(); }
