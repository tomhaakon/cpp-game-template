#pragma once

#include "game/GameStateMachine.h"
#include "game/GameWindow.h"

#include <teya/graphics/PixelCanvas.h>
#if TEYA_ENABLE_EDITOR
#include "game/GameEditorHost.h"
#include <teya/editor/Editor.h>
#include <memory>
#endif

class Game {
  public:
    Game();

    Game(const Game &) = delete;            // Copy construction
    Game &operator=(const Game &) = delete; // Copy assignment

    Game(Game &&) = delete;            // Move construction
    Game &operator=(Game &&) = delete; // Move assignment

    void run();
#if TEYA_ENABLE_EDITOR
    RenderTexture2D editorTexture() const;
    int editorCanvasWidth() const;
    int editorCanvasHeight() const;
    std::vector<teya::editor::RuntimeNode> editorHierarchy() const;
    std::vector<teya::editor::RuntimeProperty> editorProperties(teya::editor::RuntimeObjectId id) const;
    teya::editor::EditorFrameMetrics editorMetrics() const;
    Player& editorPlayer() { return gameStates_.editorPlayer(); }
    void requestGameRestart(bool pauseAfterRestart);
    void requestEditorExit();
#endif

  private:
    void update(float deltaTime);
    void draw();

    GameWindow window_;
    teya::graphics::PixelCanvas canvas_;
    GameStateMachine gameStates_;
    bool exitRequested_ = false;
#if TEYA_ENABLE_EDITOR
    std::unique_ptr<GameEditorHost> editorHost_;
    std::unique_ptr<teya::editor::Editor> editor_;
    teya::editor::EditorFrameMetrics metrics_;
    bool restartRequested_ = false;
#endif
};
