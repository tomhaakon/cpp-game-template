#include "game/Game.h"
#include <raylib.h>
#include <teya/core/Log.h>

Game::Game(int width, int height, std::string title) {
    SetConfigFlags(FLAG_WINDOW_RESIZABLE);
    InitWindow(width, height, title.c_str());
    windowOpen_ = IsWindowReady();
    SetTargetFPS(60);
    if (!canvas_.initialize(640, 360)) {
        teya::core::Log::error("Graphics", "Could not create the 640x360 pixel canvas");
    }
    (void)map_.load("assets/maps/template_map.tmj");
    teya::core::Log::info("Game", "end of Game::Game.");
}

Game::~Game() {
    if (windowOpen_) {
        map_.unload();
        canvas_.shutdown();
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
    canvas_.begin();
    ClearBackground(RAYWHITE);
    map_.draw();
    canvas_.end();

    BeginDrawing();
    ClearBackground(BLACK);
    canvas_.present(GetScreenWidth(), GetScreenHeight());
    EndDrawing();
}
