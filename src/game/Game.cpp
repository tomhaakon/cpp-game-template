#include "game/Game.h"
#include "game/GameConfig.h"
#include <raylib.h>
#include <stdexcept>
#include <chrono>
#include <teya/core/Log.h>
#include <teya/core/Profile.h>

Game::Game() : gameStates_(canvas_) {
    TEYA_PROFILE_ZONE_NAMED("Game::Game");
    if (!canvas_.initialize(GameConfig::CanvasWidth, GameConfig::CanvasHeight)) {
        throw std::runtime_error("Could not initialize the pixel canvas");
    }
    if (!gameStates_.initialize()) {
        throw std::runtime_error("Could not initialize the game world");
    }
#if TEYA_ENABLE_EDITOR
    editorHost_ = std::make_unique<GameEditorHost>(*this);
    editor_ = std::make_unique<teya::editor::Editor>(*editorHost_);
    if (!editor_->initialize()) throw std::runtime_error("Could not initialize editor");
#endif

    teya::core::Log::info("Game", "end of Game::Game.");
}

void Game::run() {
    TEYA_PROFILE_THREAD("Main");
    while (true) {
        if (exitRequested_) {
            teya::core::Log::info("Game", "Closing after an editor exit request");
            break;
        }
        if (WindowShouldClose()) {
            teya::core::Log::info("Game", "Closing after an operating-system window request");
            break;
        }
        TEYA_PROFILE_ZONE_NAMED("Game frame");
        const float deltaTime = GetFrameTime();
        TEYA_PROFILE_PLOT("Frame time (ms)", deltaTime * 1000.0f);
        const auto updateStart = std::chrono::steady_clock::now();
        update(deltaTime);
#if TEYA_ENABLE_EDITOR
        metrics_.updateMilliseconds = std::chrono::duration<float, std::milli>(
            std::chrono::steady_clock::now() - updateStart).count();
#endif
        draw();
        TEYA_PROFILE_FRAME();
    }
}

void Game::update(float deltaTime) {
    TEYA_PROFILE_ZONE_NAMED("Game::update");
#if TEYA_ENABLE_EDITOR
    const bool inputEnabled = editor_->gameInputEnabled();
    if (!editor_->shouldUpdateGame()) return;
    if (editor_->context().simulationMode() == teya::editor::SimulationMode::Paused) deltaTime = 1.0f / 60.0f;
#else
    constexpr bool inputEnabled = true;
#endif
    if (gameStates_.update(deltaTime, inputEnabled) == GameAction::Exit) {
        exitRequested_ = true;
    }
}

void Game::draw() {
    TEYA_PROFILE_ZONE_NAMED("Game::draw");
#if TEYA_ENABLE_EDITOR
    const auto gameDrawStart = std::chrono::steady_clock::now();
#endif
    canvas_.begin();
    gameStates_.draw();
    canvas_.end();
#if TEYA_ENABLE_EDITOR
    metrics_.drawMilliseconds = std::chrono::duration<float, std::milli>(
        std::chrono::steady_clock::now() - gameDrawStart).count();
#endif

    BeginDrawing();
    ClearBackground(BLACK);
#if TEYA_ENABLE_EDITOR
    const auto editorStart = std::chrono::steady_clock::now();
    editor_->draw();
    metrics_.editorMilliseconds = std::chrono::duration<float, std::milli>(
        std::chrono::steady_clock::now() - editorStart).count();
#else
    canvas_.present(GetScreenWidth(), GetScreenHeight());
#endif
    EndDrawing();
}

#if TEYA_ENABLE_EDITOR
RenderTexture2D Game::editorTexture() const{return canvas_.renderTexture();}
int Game::editorCanvasWidth() const{return canvas_.width();}
int Game::editorCanvasHeight() const{return canvas_.height();}
std::vector<teya::editor::RuntimeNode> Game::editorHierarchy() const{return {{1,0,"Game"},{2,1,gameStates_.stateName()},{3,2,"World"},{4,3,"Map"},{5,3,"Player"},{6,3,"Collision World"}};}
std::vector<teya::editor::RuntimeProperty> Game::editorProperties(teya::editor::RuntimeObjectId id) const{return gameStates_.editorProperties(id);}
teya::editor::EditorFrameMetrics Game::editorMetrics() const{auto m=metrics_;m.fps=static_cast<float>(GetFPS());m.frameMilliseconds=GetFrameTime()*1000.0f;m.canvasWidth=canvas_.width();m.canvasHeight=canvas_.height();m.imageWidth=canvas_.width();m.imageHeight=canvas_.height();m.currentState=gameStates_.stateName();m.currentMap=gameStates_.mapName();m.colliderCount=gameStates_.colliderCount();return m;}
void Game::requestEditorExit(){exitRequested_=true;}
#endif
