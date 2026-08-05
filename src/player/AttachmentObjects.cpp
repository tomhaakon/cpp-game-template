#include "player/AttachmentObjects.h"
#include <cmath>
#include <fstream>
#include <nlohmann/json.hpp>
#include <unordered_set>

namespace {
using json = nlohmann::ordered_json;
json vector(Vector2 value) { return {{"x", value.x}, {"y", value.y}}; }
Vector2 vector(const json &value, Vector2 fallback = {}) {
    return {value.value("x", fallback.x), value.value("y", fallback.y)};
}
bool validate(const std::vector<AttachmentObject> &objects, std::string &error) {
    std::unordered_set<std::uint64_t> ids;
    for (const auto &object : objects) {
        if (object.id == 0 || !ids.insert(object.id).second) {
            error = "Attachment object IDs must be non-zero and unique";
            return false;
        }
        if (object.name.empty() || object.texturePath.empty() || object.socketName.empty()) {
            error = "Attachment name, texture, and socket must not be empty";
            return false;
        }
        if (!std::isfinite(object.pivot.x) || !std::isfinite(object.pivot.y) ||
            !std::isfinite(object.positionOffset.x) || !std::isfinite(object.positionOffset.y) ||
            !std::isfinite(object.rotationOffsetDegrees) || !std::isfinite(object.scale.x) ||
            !std::isfinite(object.scale.y) || object.scale.x <= 0 || object.scale.y <= 0) {
            error = "Attachment transforms must be finite and scale must be positive";
            return false;
        }
    }
    return true;
}
} // namespace

bool loadAttachmentObjects(const std::filesystem::path &path,
                           std::vector<AttachmentObject> &objects, std::string &error) {
    try {
        std::ifstream input(path);
        if (!input) {
            error = "Could not open attachment object library: " + path.string();
            return false;
        }
        json root;
        input >> root;
        if (root.value("schemaVersion", 0) != 1 || !root.contains("objects")) {
            error = "Unsupported attachment object schema";
            return false;
        }
        std::vector<AttachmentObject> loaded;
        for (const auto &entry : root.at("objects")) {
            AttachmentObject object;
            object.id = entry.at("id").get<std::uint64_t>();
            object.name = entry.at("name").get<std::string>();
            object.texturePath = entry.at("texture").get<std::string>();
            object.socketName = entry.value("socket", "weapon_hand");
            object.pivot = vector(entry.value("pivot", json::object()));
            object.positionOffset = vector(entry.value("positionOffset", json::object()));
            object.rotationOffsetDegrees = entry.value("rotationOffsetDegrees", 0.0f);
            object.scale = vector(entry.value("scale", json::object()), {1, 1});
            object.layer = entry.value("layer", std::string{"in_front"}) == "behind"
                               ? teya::animation::AttachmentLayer::BehindOwner
                               : teya::animation::AttachmentLayer::InFrontOfOwner;
            object.visible = entry.value("visible", true);
            loaded.push_back(std::move(object));
        }
        if (!validate(loaded, error))
            return false;
        objects = std::move(loaded);
        return true;
    } catch (const std::exception &exception) {
        error = exception.what();
        return false;
    }
}

bool saveAttachmentObjects(const std::filesystem::path &path,
                           const std::vector<AttachmentObject> &objects, std::string &error) {
    if (!validate(objects, error))
        return false;
    json root = {{"schemaVersion", 1}, {"objects", json::array()}};
    for (const auto &object : objects)
        root["objects"].push_back(
            {{"id", object.id},
             {"name", object.name},
             {"texture", object.texturePath},
             {"socket", object.socketName},
             {"pivot", vector(object.pivot)},
             {"positionOffset", vector(object.positionOffset)},
             {"rotationOffsetDegrees", object.rotationOffsetDegrees},
             {"scale", vector(object.scale)},
             {"layer", object.layer == teya::animation::AttachmentLayer::BehindOwner ? "behind"
                                                                                      : "in_front"},
             {"visible", object.visible}});
    std::error_code ec;
    std::filesystem::create_directories(path.parent_path(), ec);
    std::ofstream output(path, std::ios::trunc);
    if (!output) {
        error = "Could not write attachment object library: " + path.string();
        return false;
    }
    output << root.dump(4) << '\n';
    if (!output) {
        error = "Writing attachment object library failed";
        return false;
    }
    return true;
}
