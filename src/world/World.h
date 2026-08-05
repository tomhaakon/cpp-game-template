#pragma once

#include "player/Player.h"

#include <teya/collision2d/World.h>
#include <teya/tiled/MapRenderer.h>
#if TEYA_ENABLE_EDITOR
#include <teya/editor/EditorHost.h>
#include <vector>
#endif

namespace game {

class World {
  public:
    World() = default;
    ~World();

    World(const World &) = delete;
    World &operator=(const World &) = delete;
    World(World &&) = delete;
    World &operator=(World &&) = delete;

    bool initialize();
    void shutdown();

    void update(float deltaTime, bool gameplayInputEnabled = true);
    void draw() const;
    [[nodiscard]] const char* mapName() const { return "template_map.tmj"; }
    [[nodiscard]] int colliderCount() const { return static_cast<int>(collisions_.size()); }
#if TEYA_ENABLE_EDITOR
    [[nodiscard]] std::vector<teya::editor::RuntimeProperty> playerProperties() const;
#endif

  private:
    teya::tiled::MapRenderer map_;
    teya::collision2d::World collisions_;
    Player player_;
};

} // namespace game
