#include "game/GameWindow.h"

#include "game/GameConfig.h"

#include <raylib.h>
#include <stdexcept>
#include <teya/core/Profile.h>

GameWindow::GameWindow() {
    TEYA_PROFILE_ZONE_NAMED("GameWindow::GameWindow");
    SetConfigFlags(FLAG_WINDOW_RESIZABLE);
    InitWindow(GameConfig::CanvasWidth * GameConfig::InitialWindowScale,
               GameConfig::CanvasHeight * GameConfig::InitialWindowScale,
               GameConfig::WindowTitle);
    if (!IsWindowReady()) {
        throw std::runtime_error("Could not create the game window");
    }
#if TEYA_ENABLE_EDITOR
    // Maximizing an already-created GLFW window emits the framebuffer resize
    // callback. Creating it with FLAG_WINDOW_MAXIMIZED can leave OpenGL using
    // the initial 960x640 viewport on some Windows high-DPI configurations.
    MaximizeWindow();
#endif
    SetExitKey(KEY_NULL);
    SetTargetFPS(GameConfig::TargetFps);
}

GameWindow::~GameWindow() { CloseWindow(); }
