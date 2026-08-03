#pragma once

#include <raylib.h>
#include <teya/animation/Animator.h>
#include <teya/collision2d/World.h>

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

    void update(float deltaTime);
    void draw() const;

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
