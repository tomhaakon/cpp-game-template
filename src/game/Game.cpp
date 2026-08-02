#include "game/Game.h"

#include <raylib.h>

Game::Game(int width, int height, std::string title) {
    SetConfigFlags(FLAG_WINDOW_RESIZABLE);
    InitWindow(width, height, title.c_str());
    windowOpen_ = IsWindowReady();
    SetTargetFPS(60);
}

Game::~Game() {
    if (windowOpen_) {
        CloseWindow();
    }
}

void Game::run() {
    while (!WindowShouldClose()) {
        const float deltaTime = GetFrameTime();
        update(deltaTime);
        draw();
    }
}

void Game::update(float deltaTime) { (void)deltaTime; }

void Game::draw() {
    BeginDrawing();
    ClearBackground(RAYWHITE);

    constexpr const char *message = "C++ game template is running! Get ready to rumble..";
    constexpr int fontSize = 30;
    const int textWidth = MeasureText(message, fontSize);
    DrawText(message, (GetScreenWidth() - textWidth) / 2, (GetScreenHeight() - fontSize) / 2,
             fontSize, DARKGRAY);

    EndDrawing();
}
