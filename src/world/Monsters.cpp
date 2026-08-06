#include "world/Monsters.h"
#include <algorithm>
#include <cmath>
#include <fstream>
#include <nlohmann/json.hpp>
#include <teya/core/AssetPath.h>
#include <teya/animation/AnimationIO.h>
#include <teya/animation/AnimationTransforms.h>
#include <unordered_set>

namespace {
using json = nlohmann::ordered_json;
teya::animation::AnimationControllerConfig monsterAnimationConfig(
    const teya::animation::AnimationAsset &asset) {
    teya::animation::AnimationControllerConfig config;
    if (!asset.controller.bindings.empty()) {
        config = asset.controller;

        // Side-view monsters can reuse their horizontal artwork when no
        // dedicated vertical animation has been authored. Explicit Up/Down
        // bindings always win over these generated fallbacks.
        const auto authoredBindings = config.bindings;
        const auto addVerticalFallback = [&](std::string_view action,
                                             teya::animation::AnimationDirection target,
                                             teya::animation::AnimationDirection source) {
            const auto hasTarget = std::find_if(
                config.bindings.begin(), config.bindings.end(), [&](const auto &binding) {
                    return binding.action == action && binding.direction == target;
                });
            if (hasTarget != config.bindings.end())
                return;
            const auto sourceBinding = std::find_if(
                authoredBindings.begin(), authoredBindings.end(), [&](const auto &binding) {
                    return binding.action == action && binding.direction == source;
                });
            if (sourceBinding != authoredBindings.end()) {
                auto fallback = *sourceBinding;
                fallback.direction = target;
                config.bindings.push_back(std::move(fallback));
            }
        };
        std::vector<std::string> actions;
        for (const auto &binding : authoredBindings)
            if (std::find(actions.begin(), actions.end(), binding.action) == actions.end())
                actions.push_back(binding.action);
        for (const auto &action : actions) {
            addVerticalFallback(action, teya::animation::AnimationDirection::Up,
                                teya::animation::AnimationDirection::Left);
            addVerticalFallback(action, teya::animation::AnimationDirection::Down,
                                teya::animation::AnimationDirection::Right);
        }
        return config;
    }
    config.defaultAction = "idle";
    if (asset.findClip("idle"))
        config.bindings.push_back({"idle", teya::animation::AnimationDirection::Any, "idle",
                                   teya::animation::AnimationActionMode::Looping, 0});
    if (asset.findClip("walk"))
        config.bindings.push_back({"walk", teya::animation::AnimationDirection::Any, "walk",
                                   teya::animation::AnimationActionMode::Looping, 0});
    if (asset.findClip("death"))
        config.bindings.push_back({"death", teya::animation::AnimationDirection::Any, "death",
                                   teya::animation::AnimationActionMode::OneShot, 100});
    if (asset.findClip("attack"))
        config.bindings.push_back({"attack", teya::animation::AnimationDirection::Any, "attack",
                                   teya::animation::AnimationActionMode::OneShot, 50});
    return config;
}
bool validate(const std::vector<MonsterDefinition> &definitions, std::string &error) {
    std::unordered_set<std::uint64_t> ids;
    for (const auto &monster : definitions) {
        if (!monster.id || !ids.insert(monster.id).second) {
            error = "Monster IDs must be non-zero and unique";
            return false;
        }
        if (monster.name.empty()) {
            error = "Monster names cannot be empty";
            return false;
        }
        if (!std::isfinite(monster.position.x) || !std::isfinite(monster.position.y) ||
            !std::isfinite(monster.size.x) || !std::isfinite(monster.size.y) ||
            monster.size.x <= 0 || monster.size.y <= 0 ||
            !std::isfinite(monster.moveSpeed) || monster.moveSpeed < 0 ||
            monster.maxHealth <= 0 || !std::isfinite(monster.stopDistance) ||
            monster.stopDistance < 0 || !std::isfinite(monster.attackRange) ||
            monster.attackRange < monster.stopDistance ||
            !std::isfinite(monster.attackCooldown) || monster.attackCooldown < 0 ||
            monster.attackDamage < 0 || !std::isfinite(monster.separationRadius) ||
            monster.separationRadius < 0 || !std::isfinite(monster.separationStrength) ||
            monster.separationStrength < 0 || !std::isfinite(monster.surroundRadius) ||
            monster.surroundRadius < 0) {
            error = "Monster positions must be finite and sizes must be positive";
            return false;
        }
    }
    return true;
}
} // namespace

Monsters::~Monsters() { unload(); }
void Monsters::unload() {
    instances_.clear();
    for (auto &entry : entries_)
        if (IsTextureValid(entry.texture))
            UnloadTexture(entry.texture);
    entries_.clear();
}
bool Monsters::replace(const std::vector<MonsterDefinition> &definitions, std::string &error) {
    if (!validate(definitions, error))
        return false;
    std::vector<Entry> replacement;
    replacement.reserve(definitions.size());
    for (const auto &definition : definitions) {
        std::shared_ptr<const teya::animation::AnimationAsset> asset;
        Texture2D texture{};
        if (!definition.animationAssetPath.empty()) {
            auto loaded = teya::animation::loadAnimationAsset(
                teya::core::assets::path(definition.animationAssetPath));
            if (loaded) {
                asset = loaded.asset;
                texture = LoadTexture(teya::core::assets::path(asset->texturePath).string().c_str());
                if (IsTextureValid(texture))
                    SetTextureFilter(texture,
                        teya::animation::raylibTextureFilter(asset->render.textureFilter));
                else
                    asset.reset();
            }
        }
        replacement.push_back({definition, std::move(asset), texture});
    }
    unload();
    entries_ = std::move(replacement);
    return true;
}
void Monsters::setInstances(const std::vector<WorldInstance> &instances) {
    instances_.clear();
    for (const auto &instance : instances) {
        if (instance.kind != WorldInstanceKind::Monster)
            continue;
        const auto master = std::find_if(entries_.begin(), entries_.end(), [&](const auto &entry) {
            return entry.definition.id == instance.masterId;
        });
        if (master == entries_.end())
            continue;
        teya::animation::AnimationController controller(master->animationAsset);
        if (master->animationAsset) {
            std::string ignored;
            (void)controller.configure(monsterAnimationConfig(*master->animationAsset), &ignored);
            (void)controller.setAction("idle");
        }
        instances_.push_back({instance, std::move(controller), master->definition.maxHealth,
                              false, false, false, false, 0.0f, 0.0f});
    }
}
bool Monsters::load(const std::filesystem::path &path, std::string &error) {
    try {
        std::ifstream input(path);
        if (!input) { error = "Could not open monster asset: " + path.string(); return false; }
        const auto root = json::parse(input);
        if (root.value("schemaVersion", 0) != 1) { error = "Unsupported monster schema"; return false; }
        std::vector<MonsterDefinition> definitions;
        for (const auto &entry : root.at("monsters")) {
            MonsterDefinition monster;
            monster.id = entry.at("id").get<std::uint64_t>();
            monster.name = entry.value("name", "Monster");
            monster.animationAssetPath = entry.value("animation", "");
            const auto &position = entry.at("position");
            const auto &size = entry.at("size");
            monster.position = {position.at("x").get<float>(), position.at("y").get<float>()};
            monster.size = {size.at("x").get<float>(), size.at("y").get<float>()};
            if (const auto color = entry.find("tint"); color != entry.end())
                monster.tint = {static_cast<unsigned char>(color->value("r", 220)),
                                static_cast<unsigned char>(color->value("g", 80)),
                                static_cast<unsigned char>(color->value("b", 80)), 255};
            monster.moveSpeed = entry.value("moveSpeed", 20.0f);
            monster.maxHealth = entry.value("maxHealth", 3);
            monster.stopDistance = entry.value("stopDistance", 12.0f);
            monster.attackRange = entry.value("attackRange", 18.0f);
            monster.attackCooldown = entry.value("attackCooldown", 0.8f);
            monster.attackDamage = entry.value("attackDamage", 1);
            monster.separationRadius = entry.value("separationRadius", 14.0f);
            monster.separationStrength = entry.value("separationStrength", 30.0f);
            monster.surroundRadius = entry.value("surroundRadius", 8.0f);
            definitions.push_back(std::move(monster));
        }
        return replace(definitions, error);
    } catch (const std::exception &exception) { error = exception.what(); return false; }
}
bool Monsters::save(const std::filesystem::path &path, std::string &error) const {
    const auto values = definitions();
    if (!validate(values, error)) return false;
    json root = {{"schemaVersion", 1}, {"monsters", json::array()}};
    for (const auto &monster : values)
        root["monsters"].push_back({{"id", monster.id}, {"name", monster.name},
            {"animation", monster.animationAssetPath},
            {"position", {{"x", monster.position.x}, {"y", monster.position.y}}},
            {"size", {{"x", monster.size.x}, {"y", monster.size.y}}},
            {"tint", {{"r", monster.tint.r}, {"g", monster.tint.g}, {"b", monster.tint.b}}},
            {"moveSpeed", monster.moveSpeed}, {"maxHealth", monster.maxHealth},
            {"stopDistance", monster.stopDistance}, {"attackRange", monster.attackRange},
            {"attackCooldown", monster.attackCooldown},
            {"attackDamage", monster.attackDamage},
            {"separationRadius", monster.separationRadius},
            {"separationStrength", monster.separationStrength},
            {"surroundRadius", monster.surroundRadius}});
    std::filesystem::create_directories(path.parent_path());
    std::ofstream output(path, std::ios::trunc);
    if (!output) { error = "Could not write monster asset"; return false; }
    output << root.dump(4) << '\n';
    return static_cast<bool>(output);
}
int Monsters::update(float deltaTime, Vector2 playerPosition,
                     std::optional<Rectangle> playerAttackBounds) {
    int damageToPlayer = 0;
    std::vector<Vector2> positions;
    positions.reserve(instances_.size());
    for (const auto &instance : instances_)
        positions.push_back(instance.definition.position);
    for (std::size_t instanceIndex = 0; instanceIndex < instances_.size(); ++instanceIndex) {
        auto &instance = instances_[instanceIndex];
        if (instance.dead)
            continue;
        const auto master = std::find_if(entries_.begin(), entries_.end(), [&](const auto &entry) {
            return entry.definition.id == instance.definition.masterId;
        });
        if (master == entries_.end())
            continue;
        if (instance.dying) {
            instance.animation.update(deltaTime);
            if (!instance.animation.hasTriggeredAction())
                instance.dead = true;
            continue;
        }
        Vector2 &position = instance.definition.position;
        const float dx = playerPosition.x - position.x, dy = playerPosition.y - position.y;
        const float distance = std::sqrt(dx * dx + dy * dy);
        const Rectangle bounds{position.x - master->definition.size.x * .5f,
                               position.y - master->definition.size.y,
                               master->definition.size.x, master->definition.size.y};
        if (!playerAttackBounds)
            instance.hitDuringAttack = false;
        else if (!instance.hitDuringAttack && CheckCollisionRecs(bounds, *playerAttackBounds)) {
            instance.hitDuringAttack = true;
            if (--instance.health <= 0) {
                instance.dying = instance.animation.trigger("death", true);
                instance.dead = !instance.dying;
            }
        }
        if (instance.dying || instance.dead) {
            instance.animation.update(deltaTime);
            continue;
        }
        if (distance > .001f) {
            if (std::abs(dx) >= std::abs(dy))
                instance.animation.setDirection(dx < 0 ? teya::animation::AnimationDirection::Left
                                                       : teya::animation::AnimationDirection::Right);
            else
                instance.animation.setDirection(dy < 0 ? teya::animation::AnimationDirection::Up
                                                       : teya::animation::AnimationDirection::Down);
        }
        instance.attackCooldownRemaining =
            std::max(0.0f, instance.attackCooldownRemaining - std::max(deltaTime, 0.0f));
        if (instance.attacking) {
            instance.attackTimeRemaining -= std::max(deltaTime, 0.0f);
            const bool playerEscaped = distance > master->definition.attackRange;
            if (playerEscaped || instance.attackTimeRemaining <= 0.0f) {
                instance.animation.clearTriggeredAction();
                instance.attacking = false;
                instance.attackCooldownRemaining = master->definition.attackCooldown;
            }
        }
        bool crowded = false;
        if (master->definition.separationRadius > .001f)
            for (std::size_t otherIndex = 0; otherIndex < positions.size(); ++otherIndex) {
                if (otherIndex == instanceIndex || instances_[otherIndex].dead)
                    continue;
                const float otherX = position.x - positions[otherIndex].x;
                const float otherY = position.y - positions[otherIndex].y;
                if (otherX * otherX + otherY * otherY <
                    master->definition.separationRadius * master->definition.separationRadius * .5625f) {
                    crowded = true;
                    break;
                }
            }
        if (!instance.attacking && !crowded && distance <= master->definition.attackRange &&
            instance.attackCooldownRemaining <= 0.0f) {
            instance.attacking = instance.animation.trigger("attack", true);
            if (instance.attacking) {
                float duration = 0.0f;
                if (const auto *attackClip = instance.animation.playback().currentClip())
                    for (const auto &frame : attackClip->frames)
                        duration += frame.durationSeconds;
                instance.attackTimeRemaining = std::max(.1f, duration + .1f);
            }
        }
        if (!instance.attacking) {
            constexpr float GoldenAngle = 2.39996323f;
            const float slotAngle = std::fmod(static_cast<float>(instance.definition.id) *
                                                  GoldenAngle,
                                              6.28318531f);
            const float slotRadius = std::min(master->definition.attackRange * .85f,
                                              master->definition.stopDistance +
                                                  master->definition.surroundRadius * .5f);
            const Vector2 target{playerPosition.x + std::cos(slotAngle) * slotRadius,
                                 playerPosition.y + std::sin(slotAngle) * slotRadius};
            const float targetDx = target.x - position.x;
            const float targetDy = target.y - position.y;
            const float targetDistance = std::sqrt(targetDx * targetDx + targetDy * targetDy);
            Vector2 velocity{};
            if (targetDistance > 1.0f) {
                velocity.x = targetDx / targetDistance * master->definition.moveSpeed;
                velocity.y = targetDy / targetDistance * master->definition.moveSpeed;
            }
            if (master->definition.separationRadius > .001f) {
                for (std::size_t otherIndex = 0; otherIndex < positions.size(); ++otherIndex) {
                    if (otherIndex == instanceIndex || instances_[otherIndex].dead)
                        continue;
                    float awayX = position.x - positions[otherIndex].x;
                    float awayY = position.y - positions[otherIndex].y;
                    float apart = std::sqrt(awayX * awayX + awayY * awayY);
                    if (apart >= master->definition.separationRadius)
                        continue;
                    if (apart < .001f) {
                        const float angle = std::fmod(
                            static_cast<float>(instance.definition.id * 31u +
                                               instances_[otherIndex].definition.id * 17u),
                            360.0f) * 0.0174532925f;
                        awayX = std::cos(angle);
                        awayY = std::sin(angle);
                        apart = 1.0f;
                    }
                    const float weight = 1.0f - apart / master->definition.separationRadius;
                    velocity.x += awayX / apart * master->definition.separationStrength * weight;
                    velocity.y += awayY / apart * master->definition.separationStrength * weight;
                }
            }
            const float velocityLength = std::sqrt(velocity.x * velocity.x + velocity.y * velocity.y);
            const bool moving = velocityLength > .001f;
            if (moving) {
                const float scale = std::min(1.0f, master->definition.moveSpeed / velocityLength);
                position.x += velocity.x * scale * std::max(deltaTime, 0.0f);
                position.y += velocity.y * scale * std::max(deltaTime, 0.0f);
            }
            (void)instance.animation.setAction(moving ? "walk" : "idle");
        }
        instance.animation.update(deltaTime);
        for (const auto &event : instance.animation.consumeEvents())
            if (instance.attacking && event.name == "attack_active" &&
                distance <= master->definition.attackRange)
                damageToPlayer += master->definition.attackDamage;
        if (instance.attacking && !instance.animation.hasTriggeredAction()) {
            instance.attacking = false;
            instance.attackCooldownRemaining = master->definition.attackCooldown;
        }
    }
    return damageToPlayer;
}
void Monsters::draw() const {
    for (const auto &instance : instances_) {
        if (instance.dead)
            continue;
        const auto master = std::find_if(entries_.begin(), entries_.end(), [&](const auto &entry) {
            return entry.definition.id == instance.definition.masterId;
        });
        if (master == entries_.end())
            continue;
        auto m = master->definition;
        m.position = instance.definition.position;
        const Rectangle destination{m.position.x - m.size.x * .5f,
                                    m.position.y - m.size.y, m.size.x, m.size.y};
        const auto *frame = instance.animation.playback().currentFrame();
        const auto source = frame && master->animationAsset
                                ? teya::animation::animationSourceRectangle(*master->animationAsset,
                                                                            *frame)
                                : std::nullopt;
        if (master->animationAsset) {
            const auto &shadow = master->animationAsset->render.groundShadow;
            if (shadow.enabled && shadow.color.a > 0) {
                const Vector2 center{m.position.x + shadow.offset.x,
                                     m.position.y + shadow.offset.y};
                Color edge = shadow.color;
                edge.a = static_cast<unsigned char>(edge.a * .28f);
                DrawEllipse(static_cast<int>(center.x), static_cast<int>(center.y),
                            shadow.size.x * .58f, shadow.size.y * .68f, edge);
                DrawEllipse(static_cast<int>(center.x), static_cast<int>(center.y),
                            shadow.size.x * .5f, shadow.size.y * .5f, shadow.color);
            }
        }
        if (source && IsTextureValid(master->texture)) {
            Rectangle sourceBounds = *source;
            const auto *clip = master->animationAsset->findClip(
                instance.animation.playback().currentClipName());
            if (clip && clip->mirrored)
                sourceBounds.width = -sourceBounds.width;
            DrawTexturePro(master->texture, sourceBounds, destination, {}, 0, WHITE);
        } else {
            constexpr Color MissingAssetPink{255, 0, 255, 255};
            DrawRectangleRec(destination, MissingAssetPink);
            DrawRectangleLinesEx(destination, 1, Color{90, 20, 20, 255});
        }
    }
}
std::vector<MonsterDefinition> Monsters::definitions() const {
    std::vector<MonsterDefinition> result;
    result.reserve(entries_.size());
    for (const auto &entry : entries_) result.push_back(entry.definition);
    return result;
}
