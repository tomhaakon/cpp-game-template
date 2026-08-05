#pragma once
#if TEYA_ENABLE_EDITOR
#include <teya/editor/EditorHost.h>
#include <unordered_map>
class Game;
class GameEditorHost final : public teya::editor::EditorHost {
  public:
    explicit GameEditorHost(Game &game) : game_(game) {}
    ~GameEditorHost() override;
    RenderTexture2D gameViewTexture() const override;
    int gameCanvasWidth() const override;
    int gameCanvasHeight() const override;
    std::vector<teya::editor::RuntimeNode> runtimeHierarchy() const override;
    std::vector<teya::editor::RuntimeProperty>
    inspectObject(teya::editor::RuntimeObjectId id) const override;
    teya::editor::EditorFrameMetrics frameMetrics() const override;
    std::vector<teya::editor::EditableAnimationAssetInfo> editableAnimationAssets() const override;
    teya::editor::AnimationAssetOperationResult
    createAnimationAsset(std::string_view name) override;
    teya::editor::AnimationAssetOperationResult
    duplicateAnimationAsset(std::uint64_t assetId, std::string_view name) override;
    teya::editor::AnimationAssetOperationResult
    renameAnimationAsset(std::uint64_t assetId, std::string_view name) override;
    teya::editor::AnimationAssetOperationResult
    deleteAnimationAsset(std::uint64_t assetId) override;
    teya::editor::AnimationWorkingCopyResult
    loadAnimationWorkingCopy(std::uint64_t assetId) override;
    teya::animation::AnimationValidationResult
    validateEditableAnimation(const teya::animation::AnimationAsset &asset, int textureWidth,
                              int textureHeight) const override;
    teya::editor::AnimationSaveResult
    saveAndApplyAnimationAsset(std::uint64_t assetId,
                               const teya::animation::AnimationAsset &asset) override;
    teya::editor::AnimationSaveResult
    applyAnimationAssetWithoutSaving(std::uint64_t assetId,
                                     const teya::animation::AnimationAsset &asset) override;
    std::vector<teya::editor::AttachmentPreviewInfo>
    attachmentPreviews(std::uint64_t assetId) const override;
    teya::editor::AttachmentObjectSaveResult
    saveAttachmentObjects(
        std::uint64_t assetId,
        const std::vector<teya::editor::AttachmentPreviewInfo> &objects) override;
    std::vector<std::string> animationEventSuggestions() const override;
    std::vector<std::string> animationMarkerTypeSuggestions() const override;
    std::string editorLogPath() const override;
    void flushEditorLog() override;
    void requestGameRestart(bool pauseAfterRestart) override;
    void requestExit() override;

  private:
    Game &game_;
    std::unordered_map<std::uint64_t, Texture2D> previewTextures_;
    std::unordered_map<std::uint64_t, std::string> previewTexturePaths_;
    mutable std::unordered_map<std::uint64_t, Texture2D> attachmentTextures_;
    mutable std::unordered_map<std::uint64_t, std::string> attachmentTexturePaths_;
};
#endif
