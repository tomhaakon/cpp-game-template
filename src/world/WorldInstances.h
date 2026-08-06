#pragma once
#include <cstdint>
#include <filesystem>
#include <raylib.h>
#include <string>
#include <vector>

enum class WorldInstanceKind { Player, Monster };
struct WorldInstance {
    std::uint64_t id = 0;
    WorldInstanceKind kind = WorldInstanceKind::Monster;
    std::uint64_t masterId = 0;
    Vector2 position{240.0f, 160.0f};
};

bool loadWorldInstances(const std::filesystem::path &path, std::vector<WorldInstance> &instances,
                        std::string &error);
bool saveWorldInstances(const std::filesystem::path &path,
                        const std::vector<WorldInstance> &instances, std::string &error);

