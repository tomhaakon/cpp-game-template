#pragma once
#include <filesystem>
#include <raylib.h>
#include <string>

struct PlayerColliderConfig {
    Vector2 offset{-5.0f, -10.0f};
    Vector2 size{10.0f, 10.0f};
    bool shadowVisible = true;
    Vector2 shadowOffset{0.0f, -1.0f};
    Vector2 shadowSize{16.0f, 6.0f};
    Color shadowColor{0, 0, 0, 105};
};

bool validatePlayerColliderConfig(const PlayerColliderConfig &config, std::string &error);
bool loadPlayerColliderConfig(const std::filesystem::path &path, PlayerColliderConfig &config,
                              std::string &error);
bool savePlayerColliderConfig(const std::filesystem::path &path,
                              const PlayerColliderConfig &config, std::string &error);
