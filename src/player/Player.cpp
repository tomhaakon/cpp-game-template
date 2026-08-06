#include "player/Player.h"
#include <algorithm>
#include <cmath>
#include <sstream>
#include <teya/animation/AnimationIO.h>
#include <teya/animation/AnimationTransforms.h>
#include <teya/core/AssetPath.h>
#include <teya/core/Input.h>
#include <teya/core/Log.h>
#include <teya/core/Profile.h>
namespace {
constexpr float WalkSpeed = 55, RunSpeed = 100;
std::string number(float v) {
    std::ostringstream s;
    s << v;
    return s.str();
}
std::string yes(bool v) { return v ? "true" : "false"; }
teya::animation::AnimationControllerConfig
playerAnimationConfig(const teya::animation::AnimationAsset &asset) {
    if (!asset.controller.bindings.empty())
        return asset.controller;
    using teya::animation::AnimationActionMode;
    using teya::animation::AnimationDirection;
    teya::animation::AnimationControllerConfig config;
    config.defaultAction = "idle";
    auto bind = [&](std::string action, AnimationDirection direction, std::string clip,
                    AnimationActionMode mode = AnimationActionMode::Looping, int priority = 0) {
        if (asset.findClip(clip))
            config.bindings.push_back(
                {std::move(action), direction, std::move(clip), mode, priority});
    };
    bind("idle", AnimationDirection::Any, "idle");
    bind("walk", AnimationDirection::Any, "walk");
    bind("run", AnimationDirection::Any, "run");
    bind("attack", AnimationDirection::Any, "sword_attack", AnimationActionMode::OneShot, 10);
    const struct {
        AnimationDirection direction;
        const char *suffix;
    } directions[] = {{AnimationDirection::Down, "down"},
                      {AnimationDirection::Up, "up"},
                      {AnimationDirection::Right, "right"},
                      {AnimationDirection::Left, "left"}};
    for (const auto &entry : directions) {
        bind("idle", entry.direction, "idle_" + std::string(entry.suffix));
        bind("walk", entry.direction, "walk_" + std::string(entry.suffix));
        bind("run", entry.direction, "run_" + std::string(entry.suffix));
        const std::string directionalAttack = "attack_" + std::string(entry.suffix);
        if (asset.findClip(directionalAttack))
            bind("attack", entry.direction, directionalAttack, AnimationActionMode::OneShot, 10);
        else
            bind("attack", entry.direction, "sword_attack_" + std::string(entry.suffix),
                 AnimationActionMode::OneShot, 10);
    }
    return config;
}
} // namespace
Player::~Player() { shutdown(); }
bool Player::initialize(teya::collision2d::World &world, teya::collision2d::Vector2 position) {
    TEYA_PROFILE_ZONE_NAMED("Player::initialize");
    shutdown();
    world_ = &world;
    position_ = {position.x, position.y + 5.0f};
    std::string colliderConfigError;
    if (!loadPlayerColliderConfig(teya::core::assets::path("player/player.config.json"),
                                  colliderConfig_, colliderConfigError)) {
        colliderConfig_ = {};
        teya::core::Log::warning("Player", colliderConfigError + "; using default collider");
    }
    auto loaded =
        teya::animation::loadAnimationAsset(teya::core::assets::path(animationAssetPath_));
    if (!loaded) {
        teya::core::Log::error("Player", "Animation asset failed: " + loaded.error);
        world_ = nullptr;
        return false;
    }
    animationAsset_ = loaded.asset;
    texture_ = LoadTexture(teya::core::assets::path(animationAsset_->texturePath).string().c_str());
    if (!IsTextureValid(texture_)) {
        teya::core::Log::error("Player", "Could not load animation texture");
        animationAsset_.reset();
        world_ = nullptr;
        return false;
    }
    auto validation =
        teya::animation::validateAnimationAsset(*animationAsset_, texture_.width, texture_.height);
    if (!validation.valid()) {
        teya::core::Log::error("Player",
                               "Animation frames exceed the loaded texture or are invalid");
        UnloadTexture(texture_);
        texture_ = {};
        animationAsset_.reset();
        world_ = nullptr;
        return false;
    }
    SetTextureFilter(texture_,
                     teya::animation::raylibTextureFilter(animationAsset_->render.textureFilter));
    std::string attachmentError;
    if (!loadAttachmentObjects(attachmentError))
        teya::core::Log::warning("Player", attachmentError);
    animation_.replaceAsset(animationAsset_);
    auto animationConfig = playerAnimationConfig(*animationAsset_);
    std::string controllerError;
    if (!animation_.configure(std::move(animationConfig), &controllerError)) {
        teya::core::Log::error("Player", "Animation controller failed: " + controllerError);
        shutdown();
        return false;
    }
    animation_.consumeEvents();
    collider_ = world_->add({{position_.x + colliderConfig_.offset.x,
                              position_.y + colliderConfig_.offset.y,
                              colliderConfig_.size.x, colliderConfig_.size.y},
                             1});
    return collider_ != teya::collision2d::InvalidColliderId;
}
void Player::shutdown() {
    TEYA_PROFILE_ZONE_NAMED("Player::shutdown");
    if (world_ && collider_ != teya::collision2d::InvalidColliderId)
        (void)world_->remove(collider_);
    collider_ = teya::collision2d::InvalidColliderId;
    world_ = nullptr;
    if (IsTextureValid(texture_))
        UnloadTexture(texture_);
    if (IsTextureValid(attachmentTexture_))
        UnloadTexture(attachmentTexture_);
    texture_ = {};
    attachmentTexture_ = {};
    attachmentObjects_.clear();
    equippedAttachmentId_ = 0;
    animationAsset_.reset();
    animation_.replaceAsset({});
    recentEvents_.clear();
    slashEffectSeconds_ = 0;
    swordTrailActive_ = false;
    swordTrail_.clear();
}
void Player::update(float dt, bool inputEnabled) {
    TEYA_PROFILE_ZONE_NAMED("Player::update");
    if (!world_ || collider_ == teya::collision2d::InvalidColliderId)
        return;
    using teya::core::Action;
    namespace Input = teya::core::Input;
    teya::collision2d::Vector2 d{};
    if (inputEnabled)
        d = {float(Input::isDown(Action::MoveRight)) - float(Input::isDown(Action::MoveLeft)),
             float(Input::isDown(Action::MoveDown)) - float(Input::isDown(Action::MoveUp))};
    float len = std::sqrt(d.x * d.x + d.y * d.y);
    bool moving = len > 0;
    if (moving) {
        d.x /= len;
        d.y /= len;
        if (std::abs(d.x) >= std::abs(d.y)) {
            animation_.setDirection(d.x < 0 ? teya::animation::AnimationDirection::Left
                                            : teya::animation::AnimationDirection::Right);
        } else {
            animation_.setDirection(d.y < 0 ? teya::animation::AnimationDirection::Up
                                            : teya::animation::AnimationDirection::Down);
        }
        if (d.x < 0)
            facingLeft_ = true;
        if (d.x > 0)
            facingLeft_ = false;
    }
    bool running = moving && inputEnabled && Input::isDown(Action::Run);
    float speed = running ? RunSpeed : WalkSpeed;
    const auto moved = world_->move(collider_, {d.x * speed * dt, d.y * speed * dt});
    position_.x += moved.appliedDelta.x;
    position_.y += moved.appliedDelta.y;
    if (inputEnabled && Input::isPressed(Action::Attack))
        animation_.trigger("attack", true);
    animation_.setAction(!moving ? "idle" : running ? "run" : "walk");
    animation_.update(dt);
    handleEvents();
    updateSwordTrail(dt);
    slashEffectSeconds_ = std::max(0.0f, slashEffectSeconds_ - dt);
}
void Player::handleEvents() {
    auto triggered = animation_.consumeEvents();
    for (const auto &e : triggered) {
        if (e.name == "spawn_slash" || e.name == "attack_active")
            slashEffectSeconds_ = .12f;
        else if (e.name == "attack_started")
            teya::core::Log::debug("Animation", "Attack started");
        else if (e.name == "attack_finished")
            teya::core::Log::debug("Animation", "Attack finished");
        else if (e.name == "play_sound")
            teya::core::Log::debug("Animation", "Sound event: " + e.payload);
        recentEvents_.push_back(e);
    }
    if (recentEvents_.size() > 8)
        recentEvents_.erase(recentEvents_.begin(),
                            recentEvents_.begin() +
                                static_cast<std::ptrdiff_t>(recentEvents_.size() - 8));
}
bool Player::currentClipMirrored() const {
    if (!animationAsset_)
        return false;
    const auto *clip = animationAsset_->findClip(animation_.playback().currentClipName());
    return clip != nullptr && clip->mirrored;
}
void Player::drawAttachments(teya::animation::AttachmentLayer layer, Vector2 topLeft) const {
    auto *f = animation_.playback().currentFrame();
    if (!f || !animationAsset_)
        return;
    auto source = teya::animation::animationSourceRectangle(*animationAsset_, *f);
    if (!source)
        return;
    const auto objectIt =
        std::find_if(attachmentObjects_.begin(), attachmentObjects_.end(), [&](const auto &object) {
            return object.id == equippedAttachmentId_;
        });
    for (auto socket : f->sockets) {
        const bool usesObject = objectIt != attachmentObjects_.end() && objectIt->visible &&
                                socket.name == objectIt->socketName &&
                                IsTextureValid(attachmentTexture_);
        if (socket.layer != layer || !socket.visible)
            continue;
        if (usesObject) {
            const bool mirrored = currentClipMirrored();
            if (mirrored)
                socket = teya::animation::mirrorSocket(socket, source->width);
            const auto &object = *objectIt;
            Vector2 position{topLeft.x + socket.position.x +
                                 (mirrored ? -object.positionOffset.x : object.positionOffset.x),
                             topLeft.y + socket.position.y + object.positionOffset.y};
            position = teya::animation::applyPositionPolicy(
                position, animationAsset_->render.roundAttachmentPositions);
            const float scaleX = socket.scale.x * object.scale.x;
            const float scaleY = socket.scale.y * object.scale.y;
            Rectangle sourceRect{0, 0, static_cast<float>(attachmentTexture_.width),
                                 static_cast<float>(attachmentTexture_.height)};
            if (mirrored)
                sourceRect.width = -sourceRect.width;
            Rectangle destination{position.x, position.y,
                                  attachmentTexture_.width * scaleX,
                                  attachmentTexture_.height * scaleY};
            Vector2 pivot{(mirrored ? attachmentTexture_.width - object.pivot.x : object.pivot.x) *
                              scaleX,
                          object.pivot.y * scaleY};
            const float rotation = socket.rotationDegrees +
                                   (mirrored ? -object.rotationOffsetDegrees
                                             : object.rotationOffsetDegrees);
            DrawTexturePro(attachmentTexture_, sourceRect, destination, pivot, rotation, WHITE);
            continue;
        }
        if (currentClipMirrored())
            socket = teya::animation::mirrorSocket(socket, source->width);
        Vector2 p{topLeft.x + socket.position.x, topLeft.y + socket.position.y};
        p = teya::animation::applyPositionPolicy(p,
                                                 animationAsset_->render.roundAttachmentPositions);
        float radians = socket.rotationDegrees * DEG2RAD;
        Vector2 tip{p.x + std::cos(radians) * 18 * socket.scale.x,
                    p.y + std::sin(radians) * 18 * socket.scale.y};
        if (animationAsset_->render.roundAttachmentPositions)
            tip = teya::animation::applyPositionPolicy(tip, true);
        DrawLineEx(p, tip, 3, Color{90, 58, 32, 255});
        DrawCircleV(tip, 2, LIGHTGRAY);
    }
}

std::optional<Vector2> Player::attachmentTipWorld() const {
    if (!world_ || !animationAsset_ || !IsTextureValid(attachmentTexture_))
        return std::nullopt;
    const auto *frame = animation_.playback().currentFrame();
    if (!frame)
        return std::nullopt;
    const auto source = teya::animation::animationSourceRectangle(*animationAsset_, *frame);
    if (!source)
        return std::nullopt;
    const auto object =
        std::find_if(attachmentObjects_.begin(), attachmentObjects_.end(), [&](const auto &entry) {
            return entry.id == equippedAttachmentId_;
        });
    if (object == attachmentObjects_.end() || !object->visible)
        return std::nullopt;
    auto socket = std::find_if(frame->sockets.begin(), frame->sockets.end(),
                               [&](const auto &entry) {
                                   return entry.name == object->socketName && entry.visible;
                               });
    if (socket == frame->sockets.end())
        return std::nullopt;
    const bool mirrored = currentClipMirrored();
    auto displayedSocket = mirrored ? teya::animation::mirrorSocket(*socket, source->width)
                                    : *socket;
    Vector2 topLeft{position_.x - source->width * .5f, position_.y - source->height};
    topLeft =
        teya::animation::applyPositionPolicy(topLeft, animationAsset_->render.roundOwnerPosition);
    Vector2 anchor{topLeft.x + displayedSocket.position.x +
                       (mirrored ? -object->positionOffset.x : object->positionOffset.x),
                   topLeft.y + displayedSocket.position.y + object->positionOffset.y};
    const float scaleX = displayedSocket.scale.x * object->scale.x;
    const float scaleY = displayedSocket.scale.y * object->scale.y;
    const float pivotX = mirrored ? attachmentTexture_.width - object->pivot.x : object->pivot.x;
    const float tipX = mirrored ? attachmentTexture_.width - object->effectTip.x
                                : object->effectTip.x;
    const float tipY = object->effectTip.y;
    const float localX = (tipX - pivotX) * scaleX;
    const float localY = (tipY - object->pivot.y) * scaleY;
    const float rotation =
        (displayedSocket.rotationDegrees +
         (mirrored ? -object->rotationOffsetDegrees : object->rotationOffsetDegrees)) *
        DEG2RAD;
    const float cosine = std::cos(rotation), sine = std::sin(rotation);
    return Vector2{anchor.x + localX * cosine - localY * sine,
                   anchor.y + localX * sine + localY * cosine};
}

void Player::updateSwordTrail(float deltaTime) {
    const auto object =
        std::find_if(attachmentObjects_.begin(), attachmentObjects_.end(), [&](const auto &entry) {
            return entry.id == equippedAttachmentId_;
        });
    const float trailLifetime = object == attachmentObjects_.end()
                                    ? 0.25f
                                    : object->trailLifetimeSeconds;
    for (auto &sample : swordTrail_)
        sample.ageSeconds += std::max(0.0f, deltaTime);
    swordTrail_.erase(std::remove_if(swordTrail_.begin(), swordTrail_.end(),
                                     [&](const auto &sample) {
                                         return sample.ageSeconds >= trailLifetime;
                                     }),
                      swordTrail_.end());
    const bool attacking = object != attachmentObjects_.end() && object->trailEnabled &&
                           animation_.currentAction() == "attack";
    if (attacking && !swordTrailActive_)
        swordTrail_.clear();
    swordTrailActive_ = attacking;
    if (!attacking)
        return;
    const auto tip = attachmentTipWorld();
    if (!tip)
        return;
    Vector2 smoothedTip = *tip;
    if (!swordTrail_.empty()) {
        const float smoothing = std::clamp(object->trailSmoothing, 0.0f, .95f);
        smoothedTip.x = tip->x * (1.0f - smoothing) +
                        swordTrail_.back().position.x * smoothing;
        smoothedTip.y = tip->y * (1.0f - smoothing) +
                        swordTrail_.back().position.y * smoothing;
    }
    if (!swordTrail_.empty()) {
        const float dx = smoothedTip.x - swordTrail_.back().position.x;
        const float dy = smoothedTip.y - swordTrail_.back().position.y;
        if (dx * dx + dy * dy < 0.04f)
            return;
    }
    constexpr std::size_t MaxTrailSamples = 24;
    if (swordTrail_.size() == MaxTrailSamples)
        swordTrail_.erase(swordTrail_.begin());
    swordTrail_.push_back({smoothedTip, 0.0f});
}

void Player::drawSwordTrail() const {
    if (swordTrail_.size() < 2)
        return;
    const auto object =
        std::find_if(attachmentObjects_.begin(), attachmentObjects_.end(), [&](const auto &entry) {
            return entry.id == equippedAttachmentId_;
        });
    if (object == attachmentObjects_.end() || !object->trailEnabled)
        return;
    for (std::size_t i = 1; i < swordTrail_.size(); ++i) {
        const float life = std::clamp(
            1.0f - swordTrail_[i - 1].ageSeconds / object->trailLifetimeSeconds, 0.0f, 1.0f);
        const unsigned char alpha =
            static_cast<unsigned char>(255.0f * object->trailOpacity * life);
        DrawLineEx(swordTrail_[i - 1].position, swordTrail_[i].position,
                   object->trailWidth * life + 1.0f, Color{35, 115, 240, alpha});
        DrawLineEx(swordTrail_[i - 1].position, swordTrail_[i].position,
                   std::max(1.0f, object->trailWidth * .22f),
                   Color{120, 205, 255, static_cast<unsigned char>(alpha * .65f)});
    }
}

bool Player::loadAttachmentObjects(std::string &error) {
    std::vector<AttachmentObject> objects;
    if (!::loadAttachmentObjects(teya::core::assets::path("attachments/player.attachments.json"),
                                 objects, error))
        return false;
    return replaceAttachmentObjects(std::move(objects), error);
}

bool Player::replaceAttachmentObjects(std::vector<AttachmentObject> objects, std::string &error) {
    if (objects.empty()) {
        error = "Attachment object library contains no objects";
        return false;
    }
    const std::uint64_t desired =
        std::any_of(objects.begin(), objects.end(), [&](const auto &object) {
            return object.id == equippedAttachmentId_;
        })
            ? equippedAttachmentId_
            : objects.front().id;
    const auto selected = std::find_if(objects.begin(), objects.end(),
                                       [&](const auto &object) { return object.id == desired; });
    Texture2D replacement =
        LoadTexture(teya::core::assets::path(selected->texturePath).string().c_str());
    if (!IsTextureValid(replacement)) {
        error = "Could not load attachment texture: " + selected->texturePath;
        return false;
    }
    SetTextureFilter(replacement, selected->smoothRotationFiltering
                                      ? TEXTURE_FILTER_BILINEAR
                                      : animationAsset_ ? teya::animation::raylibTextureFilter(
                                                            animationAsset_->render.textureFilter)
                                                        : TEXTURE_FILTER_POINT);
    if (IsTextureValid(attachmentTexture_))
        UnloadTexture(attachmentTexture_);
    attachmentTexture_ = replacement;
    attachmentObjects_ = std::move(objects);
    equippedAttachmentId_ = desired;
    return true;
}

bool Player::equipAttachment(std::uint64_t objectId, std::string &error) {
    if (objectId == equippedAttachmentId_)
        return true;
    auto found = std::find_if(attachmentObjects_.begin(), attachmentObjects_.end(),
                              [&](const auto &object) { return object.id == objectId; });
    if (found == attachmentObjects_.end()) {
        error = "Unknown attachment object";
        return false;
    }
    Texture2D replacement =
        LoadTexture(teya::core::assets::path(found->texturePath).string().c_str());
    if (!IsTextureValid(replacement)) {
        error = "Could not load attachment texture: " + found->texturePath;
        return false;
    }
    SetTextureFilter(replacement, found->smoothRotationFiltering
                                      ? TEXTURE_FILTER_BILINEAR
                                      : animationAsset_ ? teya::animation::raylibTextureFilter(
                                                            animationAsset_->render.textureFilter)
                                                        : TEXTURE_FILTER_POINT);
    if (IsTextureValid(attachmentTexture_))
        UnloadTexture(attachmentTexture_);
    attachmentTexture_ = replacement;
    equippedAttachmentId_ = objectId;
    return true;
}

bool Player::applyColliderConfig(const PlayerColliderConfig &config, std::string &error) {
    if (!validatePlayerColliderConfig(config, error))
        return false;
    if (!world_ || collider_ == teya::collision2d::InvalidColliderId) {
        error = "Player collider is not available";
        return false;
    }
    const teya::collision2d::Rectangle bounds{position_.x + config.offset.x,
                                              position_.y + config.offset.y, config.size.x,
                                              config.size.y};
    if (!world_->setBounds(collider_, bounds)) {
        error = "Could not update the live Player collider";
        return false;
    }
    colliderConfig_ = config;
    return true;
}
void Player::draw() const {
    TEYA_PROFILE_ZONE_NAMED("Player::draw");
    if (!world_ || !IsTextureValid(texture_) || !animationAsset_)
        return;
    auto *c = world_->get(collider_);
    auto *f = animation_.playback().currentFrame();
    if (!c || !f)
        return;
    auto source = teya::animation::animationSourceRectangle(*animationAsset_, *f);
    if (!source)
        return;
    float w = source->width, h = source->height;
    Vector2 topLeft{position_.x - w * .5f, position_.y - h};
    topLeft =
        teya::animation::applyPositionPolicy(topLeft, animationAsset_->render.roundOwnerPosition);
    drawAttachments(teya::animation::AttachmentLayer::BehindOwner, topLeft);
    Rectangle src = *source;
    const bool mirrored = currentClipMirrored();
    if (mirrored)
        src.width = -src.width;
    DrawTexturePro(texture_, src, {topLeft.x, topLeft.y, w, h}, {0, 0}, 0, WHITE);
    drawSwordTrail();
    drawAttachments(teya::animation::AttachmentLayer::InFrontOfOwner, topLeft);
    if (slashEffectSeconds_ > 0) {
        Vector2 center{topLeft.x + w * .5f, topLeft.y + h * .5f};
        DrawCircleLines(int(center.x + (mirrored ? -18 : 18)), int(center.y), 14, ORANGE);
    }
}
#if TEYA_ENABLE_EDITOR
void Player::drawDebug(const teya::editor::EditorDebugDrawSettings &settings) const {
    if (!world_ || !animationAsset_)
        return;
    const auto *collider = world_->get(collider_);
    const auto *frame = animation_.playback().currentFrame();
    if (!collider || !frame)
        return;
    if (settings.showPlayerCollider)
        DrawRectangleLinesEx({collider->bounds.x, collider->bounds.y, collider->bounds.width,
                              collider->bounds.height},
                             1.0f, RED);
    const Vector2 origin = position_;
    if (settings.showPlayerOrigin) {
        DrawLineV({origin.x - 5, origin.y}, {origin.x + 5, origin.y}, YELLOW);
        DrawLineV({origin.x, origin.y - 5}, {origin.x, origin.y + 5}, YELLOW);
        DrawCircleLines(static_cast<int>(origin.x), static_cast<int>(origin.y), 3, YELLOW);
    }
    const auto source = teya::animation::animationSourceRectangle(*animationAsset_, *frame);
    if (!source)
        return;
    Vector2 topLeft{origin.x - source->width * .5f, origin.y - source->height};
    topLeft =
        teya::animation::applyPositionPolicy(topLeft, animationAsset_->render.roundOwnerPosition);
    const bool mirrored = currentClipMirrored();
    if (settings.showAnimationHitboxes)
        for (auto hitbox : frame->hitboxes) {
            if (mirrored)
                hitbox = teya::animation::mirrorHitbox(hitbox, source->width);
            const auto &bounds = hitbox.localBounds;
            const Vector2 center{bounds.x + bounds.width * .5f,
                                 bounds.y + bounds.height * .5f};
            const float radians = hitbox.rotationDegrees * DEG2RAD;
            const float cosine = std::cos(radians), sine = std::sin(radians);
            auto worldPoint = [&](float x, float y) {
                x -= center.x;
                y -= center.y;
                return Vector2{topLeft.x + center.x + x * cosine - y * sine,
                               topLeft.y + center.y + x * sine + y * cosine};
            };
            const Vector2 corners[] = {worldPoint(bounds.x, bounds.y),
                                       worldPoint(bounds.x + bounds.width, bounds.y),
                                       worldPoint(bounds.x + bounds.width,
                                                  bounds.y + bounds.height),
                                       worldPoint(bounds.x, bounds.y + bounds.height)};
            const Color color = hitbox.active ? MAGENTA : GRAY;
            for (int corner = 0; corner < 4; ++corner)
                DrawLineV(corners[corner], corners[(corner + 1) % 4], color);
        }
    if (settings.showAnimationSockets)
        for (auto socket : frame->sockets) {
            if (mirrored)
                socket = teya::animation::mirrorSocket(socket, source->width);
            const Vector2 point{topLeft.x + socket.position.x, topLeft.y + socket.position.y};
            const float radians = socket.rotationDegrees * DEG2RAD;
            DrawCircleV(point, 2.0f, SKYBLUE);
            DrawLineV(point, {point.x + std::cos(radians) * 10.0f,
                              point.y + std::sin(radians) * 10.0f}, SKYBLUE);
        }
    if (settings.showAnimationMarkers)
        for (auto marker : frame->markers) {
            if (mirrored)
                marker = teya::animation::mirrorMarker(marker, source->width);
            const Vector2 point{topLeft.x + marker.position.x, topLeft.y + marker.position.y};
            DrawLineV({point.x - 4, point.y}, {point.x + 4, point.y}, LIME);
            DrawLineV({point.x, point.y - 4}, {point.x, point.y + 4}, LIME);
        }
}

bool Player::applyAnimationAsset(std::shared_ptr<const teya::animation::AnimationAsset> asset,
                                 std::string &error) {
    TEYA_PROFILE_ZONE_NAMED("Player::applyAnimationAsset");
    if (!asset) {
        error = "No animation asset was supplied";
        return false;
    }
    Texture2D replacement = texture_;
    bool textureChanged = !animationAsset_ || asset->texturePath != animationAsset_->texturePath;
    if (textureChanged) {
        replacement = LoadTexture(teya::core::assets::path(asset->texturePath).string().c_str());
        if (!IsTextureValid(replacement)) {
            error = "Could not load replacement animation texture";
            return false;
        }
    }
    auto validation =
        teya::animation::validateAnimationAsset(*asset, replacement.width, replacement.height);
    if (!validation.valid()) {
        if (textureChanged)
            UnloadTexture(replacement);
        error = "Replacement animation is invalid for its texture";
        return false;
    }
    teya::animation::AnimationController replacementController(asset);
    std::string controllerError;
    if (!replacementController.configure(playerAnimationConfig(*asset), &controllerError)) {
        if (textureChanged)
            UnloadTexture(replacement);
        error = "Replacement animation controller is invalid: " + controllerError;
        return false;
    }
    replacementController.setDirection(animation_.direction());
    (void)replacementController.setAction(animation_.baseAction());
    SetTextureFilter(replacement,
                     teya::animation::raylibTextureFilter(asset->render.textureFilter));
    if (IsTextureValid(attachmentTexture_)) {
        const auto equipped = std::find_if(attachmentObjects_.begin(), attachmentObjects_.end(),
                                           [&](const auto &object) {
                                               return object.id == equippedAttachmentId_;
                                           });
        SetTextureFilter(attachmentTexture_,
                         equipped != attachmentObjects_.end() &&
                                 equipped->smoothRotationFiltering
                             ? TEXTURE_FILTER_BILINEAR
                             : teya::animation::raylibTextureFilter(asset->render.textureFilter));
    }
    auto oldTexture = texture_;
    animationAsset_ = std::move(asset);
    texture_ = replacement;
    animation_ = std::move(replacementController);
    animation_.consumeEvents();
    if (textureChanged && IsTextureValid(oldTexture))
        UnloadTexture(oldTexture);
    return true;
}
std::vector<teya::editor::RuntimeProperty> Player::editorProperties() const {
    std::vector<teya::editor::RuntimeProperty> p;
    auto *c = world_ ? world_->get(collider_) : nullptr;
    if (c) {
        p.push_back({"Position", number(position_.x) + ", " + number(position_.y)});
        p.push_back({"Collider", number(c->bounds.x) + ", " + number(c->bounds.y) + ", " +
                                     number(c->bounds.width) + ", " + number(c->bounds.height)});
    }
    p.push_back({"Facing", facingLeft_ ? "Left" : "Right"});
    p.push_back({"Animation asset", animationAssetPath_});
    if (!animationAsset_)
        return p;
    auto mode =
        animationAsset_->sourceMode == teya::animation::AnimationFrameSourceMode::SpriteSheetGrid
            ? "SpriteSheetGrid"
            : "TextureAtlas";
    p.push_back({"Texture path", animationAsset_->texturePath});
    p.push_back({"Frame source mode", mode});
    p.push_back({"Render mode",
                 animationAsset_->render.mode == teya::animation::AnimationRenderMode::PixelArt
                     ? "PixelArt"
                     : "Smooth"});
    p.push_back({"Texture filter", animationAsset_->render.textureFilter ==
                                           teya::animation::AnimationTextureFilter::Nearest
                                       ? "Nearest"
                                       : "Linear"});
    const auto &playback = animation_.playback();
    p.push_back({"Animation action", std::string(animation_.currentAction())});
    p.push_back({"Current clip", std::string(playback.currentClipName())});
    p.push_back({"Current frame", std::to_string(playback.currentFrameIndex())});
    p.push_back({"Frame elapsed", number(playback.frameElapsed())});
    p.push_back({"Clip elapsed", number(playback.clipElapsed())});
    p.push_back({"Playback speed", number(playback.playbackSpeed())});
    p.push_back({"Playing", yes(playback.isPlaying())});
    p.push_back({"Paused", yes(playback.isPaused())});
    p.push_back({"Complete", yes(playback.isComplete())});
    auto *f = playback.currentFrame();
    if (f) {
        auto r = teya::animation::animationSourceRectangle(*animationAsset_, *f);
        if (r)
            p.push_back({"Source rectangle", number(r->x) + ", " + number(r->y) + ", " +
                                                 number(r->width) + ", " + number(r->height)});
        for (const auto &s : f->sockets)
            p.push_back({"Socket: " + s.name, number(s.position.x) + ", " + number(s.position.y) +
                                                  " rot " + number(s.rotationDegrees)});
        for (const auto &h : f->hitboxes)
            p.push_back({"Hitbox: " + h.name,
                         number(h.localBounds.x) + ", " + number(h.localBounds.y) + ", " +
                             number(h.localBounds.width) + ", " + number(h.localBounds.height)});
        for (const auto &m : f->markers)
            p.push_back({"Marker: " + m.name,
                         m.type + " @ " + number(m.position.x) + ", " + number(m.position.y)});
    }
    for (const auto &e : recentEvents_)
        p.push_back({"Recent event", e.clipName + "[" + std::to_string(e.frameIndex) + "] " +
                                         e.name + " " + e.payload});
    return p;
}
#endif
