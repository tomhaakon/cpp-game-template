#include "game/GameWindow.h"

#include "game/GameConfig.h"

#include <raylib.h>
#include <stdexcept>
#include <teya/core/Profile.h>

GameWindow::GameWindow() {
    TEYA_PROFILE_ZONE_NAMED("GameWindow::GameWindow");
    unsigned int windowFlags = FLAG_WINDOW_RESIZABLE;
#if TEYA_ENABLE_EDITOR
    windowFlags |= FLAG_WINDOW_MAXIMIZED;
#endif
    SetConfigFlags(windowFlags);
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
