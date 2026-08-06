#include "player/PlayerConfig.h"
#include <cmath>
#include <fstream>
#include <iomanip>
#include <nlohmann/json.hpp>

bool validatePlayerColliderConfig(const PlayerColliderConfig &config, std::string &error) {
    if (!std::isfinite(config.offset.x) || !std::isfinite(config.offset.y) ||
        !std::isfinite(config.size.x) || !std::isfinite(config.size.y)) {
        error = "Collider values must be finite";
        return false;
    }
    if (config.size.x <= 0.0f || config.size.y <= 0.0f) {
        error = "Collider width and height must be greater than zero";
        return false;
    }
    if (!std::isfinite(config.shadowOffset.x) || !std::isfinite(config.shadowOffset.y) ||
        !std::isfinite(config.shadowSize.x) || !std::isfinite(config.shadowSize.y) ||
        config.shadowSize.x <= 0.0f || config.shadowSize.y <= 0.0f) {
        error = "Shadow offset must be finite and shadow size must be greater than zero";
        return false;
    }
    return true;
}

bool loadPlayerColliderConfig(const std::filesystem::path &path, PlayerColliderConfig &config,
                              std::string &error) {
    try {
        std::ifstream input(path);
        if (!input) {
            error = "Could not open " + path.string();
            return false;
        }
        const auto json = nlohmann::ordered_json::parse(input);
        if (json.at("schemaVersion").get<int>() != 1) {
            error = "Unsupported Player config schema version";
            return false;
        }
        PlayerColliderConfig loaded;
        const auto &collider = json.at("collider");
        loaded.offset = {collider.at("offset").at("x").get<float>(),
                         collider.at("offset").at("y").get<float>()};
        loaded.size = {collider.at("size").at("x").get<float>(),
                       collider.at("size").at("y").get<float>()};
        if (const auto shadow = json.find("groundShadow"); shadow != json.end()) {
            loaded.shadowVisible = shadow->value("visible", true);
            loaded.shadowOffset = {shadow->at("offset").at("x").get<float>(),
                                   shadow->at("offset").at("y").get<float>()};
            loaded.shadowSize = {shadow->at("size").at("x").get<float>(),
                                 shadow->at("size").at("y").get<float>()};
            if (const auto color = shadow->find("color"); color != shadow->end())
                loaded.shadowColor = {
                    static_cast<unsigned char>(color->value("r", 0)),
                    static_cast<unsigned char>(color->value("g", 0)),
                    static_cast<unsigned char>(color->value("b", 0)),
                    static_cast<unsigned char>(color->value("a", 105))};
        }
        if (!validatePlayerColliderConfig(loaded, error))
            return false;
        config = loaded;
        return true;
    } catch (const std::exception &exception) {
        error = std::string("Invalid Player config: ") + exception.what();
        return false;
    }
}

bool savePlayerColliderConfig(const std::filesystem::path &path,
                              const PlayerColliderConfig &config, std::string &error) {
    if (!validatePlayerColliderConfig(config, error))
        return false;
    try {
        std::filesystem::create_directories(path.parent_path());
        nlohmann::ordered_json json;
        json["schemaVersion"] = 1;
        json["collider"] = {{"offset", {{"x", config.offset.x}, {"y", config.offset.y}}},
                            {"size", {{"x", config.size.x}, {"y", config.size.y}}}};
        json["groundShadow"] = {
            {"visible", config.shadowVisible},
            {"offset", {{"x", config.shadowOffset.x}, {"y", config.shadowOffset.y}}},
            {"size", {{"x", config.shadowSize.x}, {"y", config.shadowSize.y}}},
            {"color", {{"r", config.shadowColor.r},
                       {"g", config.shadowColor.g},
                       {"b", config.shadowColor.b},
                       {"a", config.shadowColor.a}}}};
        std::ofstream output(path, std::ios::trunc);
        if (!output) {
            error = "Could not write " + path.string();
            return false;
        }
        output << std::setw(4) << json << '\n';
        if (!output) {
            error = "Could not finish writing " + path.string();
            return false;
        }
        return true;
    } catch (const std::exception &exception) {
        error = std::string("Could not save Player config: ") + exception.what();
        return false;
    }
}
