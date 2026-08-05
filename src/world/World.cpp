#include "world/World.h"
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
    const bool playerReady =
        player_.initialize(collisions_, {WorldWidth * 0.5f, WorldHeight * 0.5f});
    return mapLoaded && playerReady;
}

void World::shutdown() {
    TEYA_PROFILE_ZONE_NAMED("World::shutdown");
    player_.shutdown();
    collisions_.clear();
    map_.unload();
}

void World::update(float deltaTime, bool gameplayInputEnabled) {
    TEYA_PROFILE_ZONE_NAMED("World::update");
    player_.update(deltaTime, gameplayInputEnabled);
}
#if TEYA_ENABLE_EDITOR
std::vector<teya::editor::RuntimeProperty> World::playerProperties() const{return player_.editorProperties();}
#endif

void World::draw() const {
    TEYA_PROFILE_ZONE_NAMED("World::draw");
    map_.draw();
    player_.draw();
}

} // namespace game
