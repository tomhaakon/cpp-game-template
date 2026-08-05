#pragma once
#include <raylib.h>
#include <teya/animation/AnimationPlayer.h>
#include <teya/collision2d/World.h>
#include <memory>
#include <string>
#include <vector>
#if TEYA_ENABLE_EDITOR
#include <teya/editor/EditorHost.h>
#endif
class Player {
public:
    Player()=default; ~Player(); Player(const Player&)=delete; Player& operator=(const Player&)=delete;
    bool initialize(teya::collision2d::World& world,teya::collision2d::Vector2 position); void shutdown();
    void update(float deltaTime,bool inputEnabled=true); void draw() const;
#if TEYA_ENABLE_EDITOR
    std::vector<teya::editor::RuntimeProperty> editorProperties() const;
#endif
private:
    enum class MovementState{Idle,Walk,Run};
    void selectMovementClip(MovementState state); void handleEvents();
    void drawAttachments(teya::animation::AttachmentLayer layer,Vector2 ownerTopLeft) const;
    teya::collision2d::World* world_=nullptr; teya::collision2d::ColliderId collider_=teya::collision2d::InvalidColliderId;
    Texture2D texture_{}; std::shared_ptr<const teya::animation::AnimationAsset> animationAsset_;
    teya::animation::AnimationPlayer animation_; MovementState movementState_=MovementState::Idle;
    bool facingLeft_=false; float slashEffectSeconds_=0; std::vector<teya::animation::TriggeredAnimationEvent> recentEvents_;
    std::string animationAssetPath_="animations/player.animation.json";
};
