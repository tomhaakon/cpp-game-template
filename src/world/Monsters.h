#pragma once
#include <cstdint>
#include <filesystem>
#include <raylib.h>
#include <string>
#include <vector>
#include <memory>
#include <optional>
#include <teya/animation/AnimationController.h>
#include "world/WorldInstances.h"

struct MonsterDefinition {
    std::uint64_t id = 0;
    std::string name = "Monster";
    std::string animationAssetPath;
    Vector2 position{240.0f, 160.0f};
    Vector2 size{16.0f, 16.0f};
    Color tint{255, 0, 255, 255};
    float moveSpeed = 20.0f;
    int maxHealth = 3;
    float stopDistance = 12.0f;
    float attackRange = 18.0f;
    float attackCooldown = 0.8f;
    int attackDamage = 1;
    float separationRadius = 14.0f;
    float separationStrength = 30.0f;
    float surroundRadius = 8.0f;
};

class Monsters {
  public:
    ~Monsters();
    bool load(const std::filesystem::path &path, std::string &error);
    bool save(const std::filesystem::path &path, std::string &error) const;
    bool replace(const std::vector<MonsterDefinition> &definitions, std::string &error);
    void unload();
    int update(float deltaTime, Vector2 playerPosition,
               std::optional<Rectangle> playerAttackBounds);
    void setInstances(const std::vector<WorldInstance> &instances);
    void draw() const;
    [[nodiscard]] std::vector<MonsterDefinition> definitions() const;

  private:
    struct Entry {
        MonsterDefinition definition;
        std::shared_ptr<const teya::animation::AnimationAsset> animationAsset;
        Texture2D texture{};
    };
    std::vector<Entry> entries_;
    struct RuntimeInstance {
        WorldInstance definition;
        teya::animation::AnimationController animation;
        int health = 1;
        bool hitDuringAttack = false;
        bool dead = false;
        bool dying = false;
        bool attacking = false;
        float attackCooldownRemaining = 0.0f;
        float attackTimeRemaining = 0.0f;
    };
    std::vector<RuntimeInstance> instances_;
};
