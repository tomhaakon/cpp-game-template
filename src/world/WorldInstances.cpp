#include "world/WorldInstances.h"
#include <cmath>
#include <fstream>
#include <nlohmann/json.hpp>
#include <unordered_set>

namespace {
using json = nlohmann::ordered_json;
bool validate(const std::vector<WorldInstance> &instances, std::string &error) {
    std::unordered_set<std::uint64_t> ids;
    int players = 0;
    for (const auto &instance : instances) {
        if (!instance.id || !ids.insert(instance.id).second) {
            error = "Instance IDs must be non-zero and unique"; return false;
        }
        if (!std::isfinite(instance.position.x) || !std::isfinite(instance.position.y)) {
            error = "Instance positions must be finite"; return false;
        }
        if (instance.kind == WorldInstanceKind::Player) ++players;
        else if (!instance.masterId) { error = "Monster instances require a master"; return false; }
    }
    if (players > 1) { error = "Only one Player instance is allowed"; return false; }
    return true;
}
}
bool loadWorldInstances(const std::filesystem::path &path, std::vector<WorldInstance> &instances,
                        std::string &error) {
    try {
        std::ifstream input(path);
        if (!input) { error = "Could not open world instances"; return false; }
        const auto root = json::parse(input);
        if (root.value("schemaVersion", 0) != 1) { error = "Unsupported instance schema"; return false; }
        std::vector<WorldInstance> loaded;
        for (const auto &entry : root.at("instances")) {
            WorldInstance value;
            value.id = entry.at("id").get<std::uint64_t>();
            value.kind = entry.at("kind").get<std::string>() == "player"
                             ? WorldInstanceKind::Player : WorldInstanceKind::Monster;
            value.masterId = entry.value("masterId", std::uint64_t{0});
            value.position = {entry.at("position").at("x").get<float>(),
                              entry.at("position").at("y").get<float>()};
            value.name = entry.value("name", std::string{});
            if (value.name.empty())
                value.name = value.kind == WorldInstanceKind::Player
                                 ? "Player"
                                 : "Monster " + std::to_string(value.id);
            loaded.push_back(value);
        }
        if (!validate(loaded, error)) return false;
        instances = std::move(loaded); return true;
    } catch (const std::exception &exception) { error = exception.what(); return false; }
}
bool saveWorldInstances(const std::filesystem::path &path,
                        const std::vector<WorldInstance> &instances, std::string &error) {
    if (!validate(instances, error)) return false;
    json root = {{"schemaVersion", 1}, {"instances", json::array()}};
    for (const auto &instance : instances)
        root["instances"].push_back({{"id", instance.id},
            {"kind", instance.kind == WorldInstanceKind::Player ? "player" : "monster"},
            {"masterId", instance.masterId},
            {"name", instance.name},
            {"position", {{"x", instance.position.x}, {"y", instance.position.y}}}});
    std::filesystem::create_directories(path.parent_path());
    std::ofstream output(path, std::ios::trunc);
    if (!output) { error = "Could not write world instances"; return false; }
    output << root.dump(4) << '\n';
    return static_cast<bool>(output);
}
