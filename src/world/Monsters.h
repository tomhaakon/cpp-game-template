#pragma once
#include <cstdint>
#include <filesystem>
#include <raylib.h>
#include <string>
#include <vector>
#include <memory>
#include <teya/animation/AnimationPlayer.h>
#include "world/WorldInstances.h"

struct MonsterDefinition {
    std::uint64_t id = 0;
    std::string name = "Monster";
    std::string animationAssetPath;
    Vector2 position{240.0f, 160.0f};
    Vector2 size{16.0f, 16.0f};
    Color tint{255, 0, 255, 255};
};

class Monsters {
  public:
    ~Monsters();
    bool load(const std::filesystem::path &path, std::string &error);
    bool save(const std::filesystem::path &path, std::string &error) const;
    bool replace(const std::vector<MonsterDefinition> &definitions, std::string &error);
    void unload();
    void update(float deltaTime);
    void setInstances(const std::vector<WorldInstance> &instances);
    void draw() const;
    [[nodiscard]] std::vector<MonsterDefinition> definitions() const;

  private:
    struct Entry {
        MonsterDefinition definition;
        std::shared_ptr<const teya::animation::AnimationAsset> animationAsset;
        teya::animation::AnimationPlayer animation;
        Texture2D texture{};
    };
    std::vector<Entry> entries_;
    struct RuntimeInstance {
        WorldInstance definition;
        teya::animation::AnimationPlayer animation;
    };
    std::vector<RuntimeInstance> instances_;
};
