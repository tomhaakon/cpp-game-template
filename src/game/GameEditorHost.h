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
    std::vector<teya::editor::EditableAnimationAssetInfo> editableAnimationAssets() const override;
    teya::editor::AnimationWorkingCopyResult loadAnimationWorkingCopy(std::uint64_t assetId) override;
    teya::animation::AnimationValidationResult validateEditableAnimation(const teya::animation::AnimationAsset& asset,int textureWidth,int textureHeight) const override;
    teya::editor::AnimationSaveResult saveAndApplyAnimationAsset(std::uint64_t assetId,const teya::animation::AnimationAsset& asset) override;
    teya::editor::AnimationSaveResult applyAnimationAssetWithoutSaving(std::uint64_t assetId,const teya::animation::AnimationAsset& asset) override;
    std::vector<std::string> animationEventSuggestions() const override;
    std::vector<std::string> animationMarkerTypeSuggestions() const override;
    void requestExit() override;
private: Game& game_;
};
#endif
