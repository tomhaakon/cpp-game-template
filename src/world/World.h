#pragma once

#include "player/Player.h"

#include <teya/collision2d/World.h>
#include <teya/tiled/MapRenderer.h>

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

    void update(float deltaTime);
    void draw() const;

  private:
    teya::tiled::MapRenderer map_;
    teya::collision2d::World collisions_;
    Player player_;
};

} // namespace game
