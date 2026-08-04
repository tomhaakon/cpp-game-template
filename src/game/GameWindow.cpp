#include "game/GameWindow.h"

#include "game/GameConfig.h"

#include <raylib.h>

GameWindow::GameWindow() {
    SetConfigFlags(FLAG_WINDOW_RESIZABLE);
    InitWindow(GameConfig::CanvasWidth * GameConfig::InitialWindowScale,
               GameConfig::CanvasHeight * GameConfig::InitialWindowScale,
               GameConfig::WindowTitle);
    open_ = IsWindowReady();
    if (open_) SetTargetFPS(GameConfig::TargetFps);
}

GameWindow::~GameWindow() {
    if (open_) CloseWindow();
}

bool GameWindow::isOpen() const { return open_; }
