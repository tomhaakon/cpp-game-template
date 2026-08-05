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
    while (!exitRequested_ && !WindowShouldClose()) {
        TEYA_PROFILE_ZONE_NAMED("Game frame");
        const float deltaTime = GetFrameTime();
        TEYA_PROFILE_PLOT("Frame time (ms)", deltaTime * 1000.0f);
        update(deltaTime);
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
    canvas_.begin();
    gameStates_.draw();
    canvas_.end();

    BeginDrawing();
    ClearBackground(BLACK);
#if TEYA_ENABLE_EDITOR
    editor_->draw();
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
teya::editor::EditorFrameMetrics Game::editorMetrics() const{auto m=metrics_;m.fps=static_cast<float>(GetFPS());m.frameMilliseconds=GetFrameTime()*1000.0f;m.canvasWidth=canvas_.width();m.canvasHeight=canvas_.height();m.currentState=gameStates_.stateName();m.currentMap=gameStates_.mapName();m.colliderCount=gameStates_.colliderCount();return m;}
void Game::requestEditorExit(){exitRequested_=true;}
#endif
