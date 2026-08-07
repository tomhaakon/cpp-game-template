#pragma once

#include "player/Player.h"
#include "gameplay/HerdChallenge.h"
#include "world/Monsters.h"
#include "world/WorldInstances.h"

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

    void update(float deltaTime, bool gameplayInputEnabled = true,
                std::optional<Vector2> canvasPointer = std::nullopt);
    void draw() const;
    [[nodiscard]] const char* mapName() const { return "template_map.tmj"; }
    [[nodiscard]] int colliderCount() const { return static_cast<int>(collisions_.size()); }
    bool advanceHerdChallenge();
    bool retryHerdChallenge();
#if TEYA_ENABLE_EDITOR
    [[nodiscard]] std::vector<teya::editor::RuntimeProperty> playerProperties() const;
    [[nodiscard]] Player& editorPlayer() { return player_; }
    [[nodiscard]] std::vector<MonsterDefinition> editorMonsters() const { return monsters_.definitions(); }
    bool replaceMonsters(const std::vector<MonsterDefinition> &monsters, std::string &error);
    bool saveMonsters(std::string &error) const;
    bool reloadMonsters(std::string &error);
    [[nodiscard]] const std::vector<WorldInstance> &editorInstances() const { return instances_; }
    bool saveAndApplyInstances(const std::vector<WorldInstance> &instances, std::string &error);
    [[nodiscard]] const HerdChallenge &herdChallenge() const { return herdChallenge_; }
    bool saveAndApplyHerdChallenge(const HerdChallengeConfig &config, std::string &error);
    void drawDebug(const teya::editor::EditorDebugDrawSettings &settings) const;
#endif

  private:
    void rebuildChallengeInstances();
    bool updateChallengeResultInput(bool gameplayInputEnabled,
                                    std::optional<Vector2> canvasPointer);
    void drawChallengeResult() const;
    teya::tiled::MapRenderer map_;
    teya::collision2d::World collisions_;
    Player player_;
    Monsters monsters_;
    std::vector<WorldInstance> instances_;
    std::vector<WorldInstance> runtimeInstances_;
    HerdChallenge herdChallenge_;
    std::uint64_t nextChallengeInstanceId_ = 1000;
};

} // namespace game
