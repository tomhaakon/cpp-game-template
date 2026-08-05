#pragma once
#include <memory>
#include <raylib.h>
#include <string>
#include <teya/animation/AnimationController.h>
#include <teya/collision2d/World.h>
#include <vector>
#if TEYA_ENABLE_EDITOR
#include <teya/editor/EditorHost.h>
#endif
class Player {
  public:
    Player() = default;
    ~Player();
    Player(const Player &) = delete;
    Player &operator=(const Player &) = delete;
    bool initialize(teya::collision2d::World &world, teya::collision2d::Vector2 position);
    void shutdown();
    void update(float deltaTime, bool inputEnabled = true);
    void draw() const;
#if TEYA_ENABLE_EDITOR
    std::vector<teya::editor::RuntimeProperty> editorProperties() const;
    [[nodiscard]] const std::shared_ptr<const teya::animation::AnimationAsset> &
    animationAsset() const {
        return animationAsset_;
    }
    [[nodiscard]] Texture2D animationTexture() const { return texture_; }
    [[nodiscard]] const std::string &animationAssetPath() const { return animationAssetPath_; }
    bool applyAnimationAsset(std::shared_ptr<const teya::animation::AnimationAsset> asset,
                             std::string &error);
#endif
  private:
    void handleEvents();
    [[nodiscard]] bool currentClipMirrored() const;
    void drawAttachments(teya::animation::AttachmentLayer layer, Vector2 ownerTopLeft) const;
    teya::collision2d::World *world_ = nullptr;
    teya::collision2d::ColliderId collider_ = teya::collision2d::InvalidColliderId;
    Texture2D texture_{};
    std::shared_ptr<const teya::animation::AnimationAsset> animationAsset_;
    teya::animation::AnimationController animation_;
    bool facingLeft_ = false;
    float slashEffectSeconds_ = 0;
    std::vector<teya::animation::TriggeredAnimationEvent> recentEvents_;
    std::string animationAssetPath_ = "animations/player.animation.json";
};
