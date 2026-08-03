#include "player/Player.h"

#include <teya/core/Input.h>
#include <teya/core/Log.h>

#include <cmath>

namespace {
constexpr int IdleFrameCount = 4;
constexpr int MovementFrameCount = 6;
constexpr int AnimationRows = 3;
constexpr float SpriteFrameWidth = 48.0f;
constexpr float SpriteFrameHeight = 48.0f;
constexpr float DrawWidth = 48.0f;
constexpr float DrawHeight = 48.0f;
constexpr float ColliderSize = 10.0f;
constexpr float WalkSpeed = 55.0f;
constexpr float RunSpeed = 100.0f;
constexpr int ExpectedSheetWidth =
    MovementFrameCount * static_cast<int>(SpriteFrameWidth);
constexpr int ExpectedSheetHeight =
    AnimationRows * static_cast<int>(SpriteFrameHeight);
} // namespace

Player::~Player() { shutdown(); }

bool Player::initialize(teya::collision2d::World &world,
                        teya::collision2d::Vector2 position) {
    shutdown();
    world_ = &world;

    texture_ = LoadTexture("assets/textures/player-sheet.png");
    if (!IsTextureValid(texture_)) {
        teya::core::Log::error("Player", "Could not load the player sprite sheet");
        world_ = nullptr;
        return false;
    }

    if (texture_.width != ExpectedSheetWidth ||
        texture_.height != ExpectedSheetHeight) {
        teya::core::Log::error(
            "Player",
            "Player sprite sheet must be exactly 288x144 pixels: "
            "6 columns by 3 rows, with 48x48 frames");
        UnloadTexture(texture_);
        texture_ = {};
        world_ = nullptr;
        return false;
    }
    SetTextureFilter(texture_, TEXTURE_FILTER_POINT);

    animator_.setGridAnimation("idle", 0, IdleFrameCount, SpriteFrameWidth,
                               SpriteFrameHeight, 0.18f);
    animator_.setGridAnimation("walk", 1, MovementFrameCount, SpriteFrameWidth,
                               SpriteFrameHeight, 0.11f);
    animator_.setGridAnimation("run", 2, MovementFrameCount, SpriteFrameWidth,
                               SpriteFrameHeight, 0.075f);

    collider_ = world_->add(
        {{position.x - ColliderSize * 0.5f, position.y - ColliderSize * 0.5f,
          ColliderSize, ColliderSize}, 1});
    animationState_ = AnimationState::Idle;
    (void)animator_.play("idle", true);
    return true;
}

void Player::shutdown() {
    if (world_ && collider_ != teya::collision2d::InvalidColliderId) {
        (void)world_->remove(collider_);
    }
    collider_ = teya::collision2d::InvalidColliderId;
    world_ = nullptr;

    if (IsTextureValid(texture_)) {
        UnloadTexture(texture_);
    }
    texture_ = {};
}

void Player::update(float deltaTime) {
    if (!world_ || collider_ == teya::collision2d::InvalidColliderId) return;

    using teya::core::Action;
    namespace Input = teya::core::Input;

    teya::collision2d::Vector2 direction{
        static_cast<float>(Input::isDown(Action::MoveRight)) -
            static_cast<float>(Input::isDown(Action::MoveLeft)),
        static_cast<float>(Input::isDown(Action::MoveDown)) -
            static_cast<float>(Input::isDown(Action::MoveUp))};

    const float length = std::sqrt(direction.x * direction.x +
                                   direction.y * direction.y);
    const bool moving = length > 0.0f;
    if (moving) {
        direction.x /= length;
        direction.y /= length;
        if (direction.x < 0.0f) facingLeft_ = true;
        if (direction.x > 0.0f) facingLeft_ = false;
    }

    const bool running = moving && Input::isDown(Action::Run);
    const float speed = running ? RunSpeed : WalkSpeed;
    (void)world_->move(
        collider_, {direction.x * speed * deltaTime,
                    direction.y * speed * deltaTime});

    play(!moving ? AnimationState::Idle
                 : (running ? AnimationState::Run : AnimationState::Walk));
    animator_.update(deltaTime);
}

void Player::draw() const {
    if (!world_ || !IsTextureValid(texture_)) return;

    const auto *collider = world_->get(collider_);
    if (!collider) return;

    const Rectangle destination{
        collider->bounds.x + collider->bounds.width * 0.5f - DrawWidth * 0.5f,
        collider->bounds.y + collider->bounds.height - DrawHeight,
        DrawWidth, DrawHeight};
    animator_.draw(texture_, destination, {}, 0.0f, WHITE, facingLeft_);
}

void Player::play(AnimationState state) {
    if (animationState_ == state) return;

    animationState_ = state;
    switch (state) {
    case AnimationState::Idle:
        (void)animator_.play("idle");
        break;
    case AnimationState::Walk:
        (void)animator_.play("walk");
        break;
    case AnimationState::Run:
        (void)animator_.play("run");
        break;
    }
}
