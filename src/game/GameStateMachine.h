#pragma once

#include "ui/MainMenu.h"
#include "ui/PauseMenu.h"
#include "world/World.h"

#include <teya/graphics/PixelCanvas.h>
#include <optional>
#if TEYA_ENABLE_EDITOR
#include <teya/editor/EditorHost.h>
#include <vector>
#endif

enum class GameAction { None, Exit };

class GameStateMachine {
  public:
    explicit GameStateMachine(const teya::graphics::PixelCanvas &canvas);

    [[nodiscard]] bool initialize();
    [[nodiscard]] bool restart();
    [[nodiscard]] GameAction update(float deltaTime, bool gameplayInputEnabled = true,
                                    std::optional<Vector2> canvasPointer = std::nullopt);
    void draw() const;
    [[nodiscard]] const char* stateName() const;
    [[nodiscard]] const char* mapName() const;
    [[nodiscard]] int colliderCount() const;
#if TEYA_ENABLE_EDITOR
    [[nodiscard]] std::vector<teya::editor::RuntimeProperty> editorProperties(teya::editor::RuntimeObjectId id) const;
    [[nodiscard]] Player& editorPlayer() { return world_.editorPlayer(); }
    [[nodiscard]] game::World& editorWorld() { return world_; }
    void drawDebug(const teya::editor::EditorDebugDrawSettings &settings) const;
#endif

  private:
    enum class GameState { MainMenu, Playing, PauseMenu };

    [[nodiscard]] GameAction updateMainMenu(bool gameplayInputEnabled,
                                            std::optional<Vector2> canvasPointer);
    [[nodiscard]] GameAction updatePlaying(float deltaTime, bool gameplayInputEnabled,
                                           std::optional<Vector2> canvasPointer);
    [[nodiscard]] GameAction updatePauseMenu(bool gameplayInputEnabled,
                                             std::optional<Vector2> canvasPointer);

    game::World world_;
    game::ui::MainMenu mainMenu_;
    game::ui::PauseMenu pauseMenu_;
    GameState currentState_ = GameState::MainMenu;
};
