#pragma once

#include <raylib.h>
#include <teya/animation/Animator.h>
#include <teya/collision2d/World.h>
#if TEYA_ENABLE_EDITOR
#include <teya/editor/EditorHost.h>
#include <vector>
#endif

class Player {
  public:
    Player() = default;
    ~Player();

    Player(const Player &) = delete;
    Player &operator=(const Player &) = delete;
    Player(Player &&) = delete;
    Player &operator=(Player &&) = delete;

    bool initialize(teya::collision2d::World &world,
                    teya::collision2d::Vector2 position);
    void shutdown();

    void update(float deltaTime, bool inputEnabled = true);
    void draw() const;
#if TEYA_ENABLE_EDITOR
    [[nodiscard]] std::vector<teya::editor::RuntimeProperty> editorProperties() const;
#endif

  private:
    enum class AnimationState { Idle, Walk, Run };

    void play(AnimationState state);

    teya::collision2d::World *world_ = nullptr;
    teya::collision2d::ColliderId collider_ =
        teya::collision2d::InvalidColliderId;
    Texture2D texture_{};
    teya::animation::Animator animator_;
    AnimationState animationState_ = AnimationState::Idle;
    bool facingLeft_ = false;
};
