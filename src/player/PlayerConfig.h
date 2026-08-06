#pragma once
#include <filesystem>
#include <raylib.h>
#include <string>

struct PlayerColliderConfig {
    Vector2 offset{-5.0f, -10.0f};
    Vector2 size{10.0f, 10.0f};
};

bool validatePlayerColliderConfig(const PlayerColliderConfig &config, std::string &error);
bool loadPlayerColliderConfig(const std::filesystem::path &path, PlayerColliderConfig &config,
                              std::string &error);
bool savePlayerColliderConfig(const std::filesystem::path &path,
                              const PlayerColliderConfig &config, std::string &error);

