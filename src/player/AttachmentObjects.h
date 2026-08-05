#pragma once
#include <cstdint>
#include <filesystem>
#include <raylib.h>
#include <string>
#include <teya/animation/AnimationAsset.h>
#include <vector>

struct AttachmentObject {
    std::uint64_t id = 0;
    std::string name;
    std::string texturePath;
    std::string socketName = "weapon_hand";
    Vector2 pivot{};
    Vector2 positionOffset{};
    float rotationOffsetDegrees = 0.0f;
    Vector2 scale{1.0f, 1.0f};
    teya::animation::AttachmentLayer layer =
        teya::animation::AttachmentLayer::InFrontOfOwner;
    bool visible = true;
};

bool loadAttachmentObjects(const std::filesystem::path &path,
                           std::vector<AttachmentObject> &objects, std::string &error);
bool saveAttachmentObjects(const std::filesystem::path &path,
                           const std::vector<AttachmentObject> &objects, std::string &error);
