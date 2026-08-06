#pragma once
#include "player/AttachmentObjects.h"
#include "player/PlayerConfig.h"
#include <cstdint>
#include <memory>
#include <optional>
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
    [[nodiscard]] const std::vector<AttachmentObject> &attachmentObjects() const {
        return attachmentObjects_;
    }
    bool replaceAttachmentObjects(std::vector<AttachmentObject> objects, std::string &error);
    bool equipAttachment(std::uint64_t objectId, std::string &error);
    [[nodiscard]] Vector2 position() const { return position_; }
    [[nodiscard]] const PlayerColliderConfig &colliderConfig() const { return colliderConfig_; }
    bool applyColliderConfig(const PlayerColliderConfig &config, std::string &error);
#if TEYA_ENABLE_EDITOR
    void drawDebug(const teya::editor::EditorDebugDrawSettings &settings) const;
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
    struct SwordTrailSample {
        Vector2 position{};
        float ageSeconds = 0.0f;
    };
    void handleEvents();
    bool loadAttachmentObjects(std::string &error);
    [[nodiscard]] std::optional<Vector2> attachmentTipWorld() const;
    void updateSwordTrail(float deltaTime);
    void drawSwordTrail() const;
    [[nodiscard]] bool currentClipMirrored() const;
    void drawAttachments(teya::animation::AttachmentLayer layer, Vector2 ownerTopLeft) const;
    teya::collision2d::World *world_ = nullptr;
    teya::collision2d::ColliderId collider_ = teya::collision2d::InvalidColliderId;
    Vector2 position_{};
    PlayerColliderConfig colliderConfig_{};
    Texture2D texture_{};
    Texture2D attachmentTexture_{};
    std::vector<AttachmentObject> attachmentObjects_;
    std::uint64_t equippedAttachmentId_ = 0;
    std::shared_ptr<const teya::animation::AnimationAsset> animationAsset_;
    teya::animation::AnimationController animation_;
    bool facingLeft_ = false;
    float slashEffectSeconds_ = 0;
    bool swordTrailActive_ = false;
    std::vector<SwordTrailSample> swordTrail_;
    std::vector<teya::animation::TriggeredAnimationEvent> recentEvents_;
    std::string animationAssetPath_ = "animations/player.animation.json";
};
