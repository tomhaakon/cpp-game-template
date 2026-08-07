#include "world/World.h"
#include <algorithm>
#include <teya/core/AssetPath.h>
#include <teya/core/Log.h>
#include <teya/core/Profile.h>
#include <teya/core/Input.h>
#include "game/GameConfig.h"
#include "ui/UiStyle.h"
#include <cmath>

namespace {
constexpr float WorldWidth = 480.0f;
constexpr float WorldHeight = 320.0f;
constexpr float WorldMargin = 16.0f;
constexpr float BoundaryThickness = 8.0f;

void addBoundary(teya::collision2d::World &world,
                 teya::collision2d::Rectangle area, float thickness) {
    (void)world.add(
        {{area.x - thickness, area.y - thickness,
          area.width + thickness * 2.0f, thickness}, 2});
    (void)world.add(
        {{area.x - thickness, area.y + area.height,
          area.width + thickness * 2.0f, thickness}, 2});
    (void)world.add({{area.x - thickness, area.y, thickness, area.height}, 2});
    (void)world.add(
        {{area.x + area.width, area.y, thickness, area.height}, 2});
}
} // namespace

namespace game {

World::~World() { shutdown(); }

bool World::initialize() {
    TEYA_PROFILE_ZONE_NAMED("World::initialize");
    shutdown();

    const bool mapLoaded = map_.load("assets/maps/template_map.tmj");
    addBoundary(collisions_,
                {WorldMargin, WorldMargin, WorldWidth - WorldMargin * 2.0f,
                 WorldHeight - WorldMargin * 2.0f},
                BoundaryThickness);
    std::string instanceError;
    if (!loadWorldInstances(teya::core::assets::path("world/instances.json"), instances_,
                            instanceError)) {
        instances_ = {{1, WorldInstanceKind::Player, 0,
                       {WorldWidth * .5f, WorldHeight * .5f + 5.0f}}};
        teya::core::Log::warning("Instances", instanceError);
    }
    Vector2 playerPosition{WorldWidth * .5f, WorldHeight * .5f + 5.0f};
    const auto playerInstance = std::find_if(instances_.begin(), instances_.end(), [](const auto &i) {
        return i.kind == WorldInstanceKind::Player;
    });
    if (playerInstance != instances_.end()) playerPosition = playerInstance->position;
    const bool playerReady = player_.initialize(
        collisions_, {playerPosition.x, playerPosition.y});
    std::string monsterError;
    if (!monsters_.load(teya::core::assets::path("monsters/monsters.json"), monsterError))
        teya::core::Log::warning("Monsters", monsterError);
    std::string challengeError;
    if (!herdChallenge_.load(teya::core::assets::path("gameplay/herd-challenge.json"),
                             challengeError))
        teya::core::Log::warning("HerdChallenge", challengeError);
    runtimeInstances_ = instances_;
    rebuildChallengeInstances();
    return mapLoaded && playerReady;
}

void World::shutdown() {
    TEYA_PROFILE_ZONE_NAMED("World::shutdown");
    player_.shutdown();
    monsters_.unload();
    instances_.clear();
    runtimeInstances_.clear();
    collisions_.clear();
    map_.unload();
}

void World::update(float deltaTime, bool gameplayInputEnabled,
                   std::optional<Vector2> canvasPointer) {
    TEYA_PROFILE_ZONE_NAMED("World::update");
    const auto challengeState = herdChallenge_.state();
    if (challengeState == HerdChallengeState::LevelComplete ||
        challengeState == HerdChallengeState::Missed) {
        herdChallenge_.updateResultDelay(deltaTime);
        if (herdChallenge_.resultReady()) {
            monsters_.updateDeathAnimations(deltaTime);
            (void)updateChallengeResultInput(gameplayInputEnabled, canvasPointer);
            return;
        }
    }
    const auto &challengeConfig = herdChallenge_.config();
    player_.update(deltaTime, gameplayInputEnabled,
                   !challengeConfig.enabled || !challengeConfig.oneHitChallenge);
    (void)herdChallenge_.updateBeforeCombat(
        deltaTime, player_.movedThisFrame(), player_.isAttacking(),
        [&] { return player_.triggerAttack(); }, monsters_.aliveCount());
    auto combat = monsters_.update(deltaTime, player_.position(), player_.attackBounds(),
                                   herdChallenge_.config().enabled);
    player_.takeDamage(combat.damageToPlayer);
    herdChallenge_.updateAfterCombat(deltaTime, player_.isAttacking(), combat.hitInstanceIds);
    if (herdChallenge_.consumeSpawnRequest()) rebuildChallengeInstances();
}
bool World::updateChallengeResultInput(bool inputEnabled,
                                       std::optional<Vector2> canvasPointer) {
    const auto state = herdChallenge_.state();
    if (state != HerdChallengeState::LevelComplete && state != HerdChallengeState::Missed)
        return false;
    if (!inputEnabled) return true;
    constexpr Rectangle Button{160, 205, 160, 34};
    const auto pointer = teya::core::Input::pointerPosition();
    const Vector2 point = canvasPointer.value_or(Vector2{pointer.x, pointer.y});
    const bool clicked = teya::core::Input::isPressed(teya::core::PointerButton::Primary) &&
                         CheckCollisionPointRec(point, Button);
    const bool confirmed = teya::core::Input::isPressed(teya::core::Action::Confirm);
    if (clicked || confirmed) {
        if (state == HerdChallengeState::LevelComplete) (void)advanceHerdChallenge();
        else (void)retryHerdChallenge();
    }
    return true;
}
void World::rebuildChallengeInstances() {
    if (!herdChallenge_.config().enabled) {
        runtimeInstances_ = instances_;
        monsters_.setInstances(runtimeInstances_);
        return;
    }
    runtimeInstances_.clear();
    const auto playerInstance = std::find_if(instances_.begin(), instances_.end(), [](const auto &i) {
        return i.kind == WorldInstanceKind::Player;
    });
    if (playerInstance != instances_.end()) runtimeInstances_.push_back(*playerInstance);
    const auto sourceMonster = std::find_if(instances_.begin(), instances_.end(), [](const auto &i) {
        return i.kind == WorldInstanceKind::Monster;
    });
    const auto definitions = monsters_.definitions();
    const std::uint64_t masterId = sourceMonster != instances_.end()
                                       ? sourceMonster->masterId
                                       : definitions.empty() ? 0 : definitions.front().id;
    if (!masterId) { monsters_.setInstances(runtimeInstances_); return; }
    const Vector2 center = player_.position();
    const int count = herdChallenge_.requestedMonsterCount();
    for (int i = 0; i < count; ++i) {
        const float angle = count == 1 ? 0.0f : 6.28318531f * static_cast<float>(i) / count;
        const float radius = 65.0f + 12.0f * static_cast<float>(i / 8);
        runtimeInstances_.push_back({nextChallengeInstanceId_++,
                                     WorldInstanceKind::Monster, masterId,
                                     {center.x + std::cos(angle) * radius,
                                      center.y + std::sin(angle) * radius},
                                     "Challenge Slime " + std::to_string(i + 1)});
    }
    monsters_.setInstances(runtimeInstances_);
    herdChallenge_.beginRound(monsters_.aliveCount());
}
#if TEYA_ENABLE_EDITOR
std::vector<teya::editor::RuntimeProperty> World::playerProperties() const{return player_.editorProperties();}
void World::drawDebug(const teya::editor::EditorDebugDrawSettings &settings) const {
    if (settings.showWorldBounds)
        DrawRectangleLinesEx({WorldMargin, WorldMargin, WorldWidth - WorldMargin * 2.0f,
                              WorldHeight - WorldMargin * 2.0f},
                             1.0f, GREEN);
    player_.drawDebug(settings);
}
#endif

void World::draw() const {
    TEYA_PROFILE_ZONE_NAMED("World::draw");
    map_.draw();
    monsters_.draw();
    player_.draw();
    drawChallengeResult();
}
void World::drawChallengeResult() const {
    const auto state = herdChallenge_.state();
    if ((state != HerdChallengeState::LevelComplete && state != HerdChallengeState::Missed) ||
        !herdChallenge_.resultReady()) return;
    using namespace game::ui;
    DrawRectangle(0, 0, GameConfig::CanvasWidth, GameConfig::CanvasHeight, {0, 0, 0, 145});
    constexpr Rectangle Panel{100, 90, 280, 160};
    constexpr Rectangle Button{160, 205, 160, 34};
    DrawRectangleRec(Panel, style::Panel);
    DrawRectangleLinesEx(Panel, 2, style::Accent);
    const char *title = state == HerdChallengeState::LevelComplete ? "LEVEL CLEARED!" : "SWING MISSED";
    DrawText(title, 240 - MeasureText(title, 24) / 2, 108, 24, style::Text);
    const std::string levelText = "Level " + std::to_string(herdChallenge_.level());
    DrawText(levelText.c_str(), 240 - MeasureText(levelText.c_str(), 16) / 2, 139, 16,
             style::Accent);
    const std::string score = std::to_string(herdChallenge_.hitCount()) + " / " +
                              std::to_string(herdChallenge_.requiredHits()) + " slimes hit";
    DrawText(score.c_str(), 240 - MeasureText(score.c_str(), 18) / 2, 165, 18,
             style::MutedText);
    const char *button = state == HerdChallengeState::LevelComplete ? "NEXT LEVEL" : "RETRY";
    DrawRectangleRec(Button, style::Accent);
    DrawText(button, 240 - MeasureText(button, style::ButtonFontSize) / 2,
             static_cast<int>(Button.y) + 7, style::ButtonFontSize, style::Background);
}

#if TEYA_ENABLE_EDITOR
bool World::saveMonsters(std::string &error) const {
    return monsters_.save(teya::core::assets::path("monsters/monsters.json"), error);
}
bool World::reloadMonsters(std::string &error) {
    if (!monsters_.load(teya::core::assets::path("monsters/monsters.json"), error)) return false;
    rebuildChallengeInstances();
    return true;
}
bool World::replaceMonsters(const std::vector<MonsterDefinition> &monsters, std::string &error) {
    if (!monsters_.replace(monsters, error)) return false;
    rebuildChallengeInstances();
    return true;
}
bool World::saveAndApplyInstances(const std::vector<WorldInstance> &instances,
                                  std::string &error) {
    if (!saveWorldInstances(teya::core::assets::path("world/instances.json"), instances, error))
        return false;
    instances_ = instances;
    rebuildChallengeInstances();
    return true;
}
bool World::saveAndApplyHerdChallenge(const HerdChallengeConfig &config, std::string &error) {
    HerdChallenge validated = herdChallenge_;
    if (!validated.apply(config, error)) return false;
    if (!validated.save(teya::core::assets::path("gameplay/herd-challenge.json"), error))
        return false;
    herdChallenge_ = std::move(validated);
    rebuildChallengeInstances();
    (void)herdChallenge_.consumeSpawnRequest();
    return true;
}
#endif
bool World::advanceHerdChallenge() {
    if (!herdChallenge_.nextLevel()) return false;
    rebuildChallengeInstances();
    (void)herdChallenge_.consumeSpawnRequest();
    return true;
}
bool World::retryHerdChallenge() {
    if (!herdChallenge_.retryLevel()) return false;
    monsters_.clearCorpses();
    rebuildChallengeInstances();
    (void)herdChallenge_.consumeSpawnRequest();
    return true;
}

} // namespace game
