#include "game/Game.h"
#include <raylib.h>
#include <teya/core/Log.h>

Game::Game(int width, int height, std::string title) {
    SetConfigFlags(FLAG_WINDOW_RESIZABLE);
    InitWindow(width, height, title.c_str());
    windowOpen_ = IsWindowReady();
    SetTargetFPS(60);
    (void)map_.load("assets/maps/template_map.tmj");
    teya::core::Log::info("Game", "end of Game::Game.");
}

Game::~Game() {
    if (windowOpen_) {
        map_.unload();
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

    map_.draw();

    EndDrawing();
}
