#pragma once
#if TEYA_ENABLE_EDITOR
#include <teya/editor/EditorHost.h>
class Game;
class GameEditorHost final : public teya::editor::EditorHost {
public:
    explicit GameEditorHost(Game& game) : game_(game) {}
    RenderTexture2D gameViewTexture() const override;
    int gameCanvasWidth() const override;
    int gameCanvasHeight() const override;
    std::vector<teya::editor::RuntimeNode> runtimeHierarchy() const override;
    std::vector<teya::editor::RuntimeProperty> inspectObject(teya::editor::RuntimeObjectId id) const override;
    teya::editor::EditorFrameMetrics frameMetrics() const override;
    void requestExit() override;
private: Game& game_;
};
#endif
