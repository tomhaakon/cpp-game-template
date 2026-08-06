#include "world/World.h"
#include <algorithm>
#include <teya/core/AssetPath.h>
#include <teya/core/Log.h>
#include <teya/core/Profile.h>

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
    monsters_.setInstances(instances_);
    return mapLoaded && playerReady;
}

void World::shutdown() {
    TEYA_PROFILE_ZONE_NAMED("World::shutdown");
    player_.shutdown();
    monsters_.unload();
    instances_.clear();
    collisions_.clear();
    map_.unload();
}

void World::update(float deltaTime, bool gameplayInputEnabled) {
    TEYA_PROFILE_ZONE_NAMED("World::update");
    player_.update(deltaTime, gameplayInputEnabled);
    monsters_.update(deltaTime);
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
}

#if TEYA_ENABLE_EDITOR
bool World::saveMonsters(std::string &error) const {
    return monsters_.save(teya::core::assets::path("monsters/monsters.json"), error);
}
bool World::reloadMonsters(std::string &error) {
    if (!monsters_.load(teya::core::assets::path("monsters/monsters.json"), error)) return false;
    monsters_.setInstances(instances_);
    return true;
}
bool World::replaceMonsters(const std::vector<MonsterDefinition> &monsters, std::string &error) {
    if (!monsters_.replace(monsters, error)) return false;
    monsters_.setInstances(instances_);
    return true;
}
bool World::saveAndApplyInstances(const std::vector<WorldInstance> &instances,
                                  std::string &error) {
    if (!saveWorldInstances(teya::core::assets::path("world/instances.json"), instances, error))
        return false;
    instances_ = instances;
    monsters_.setInstances(instances_);
    return true;
}
#endif

} // namespace game
