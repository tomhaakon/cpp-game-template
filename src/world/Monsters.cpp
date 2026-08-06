#include "world/Monsters.h"
#include <algorithm>
#include <cmath>
#include <fstream>
#include <nlohmann/json.hpp>
#include <teya/core/AssetPath.h>
#include <teya/animation/AnimationIO.h>
#include <teya/animation/AnimationTransforms.h>
#include <unordered_set>

namespace {
using json = nlohmann::ordered_json;
bool validate(const std::vector<MonsterDefinition> &definitions, std::string &error) {
    std::unordered_set<std::uint64_t> ids;
    for (const auto &monster : definitions) {
        if (!monster.id || !ids.insert(monster.id).second) {
            error = "Monster IDs must be non-zero and unique";
            return false;
        }
        if (monster.name.empty()) {
            error = "Monster names cannot be empty";
            return false;
        }
        if (!std::isfinite(monster.position.x) || !std::isfinite(monster.position.y) ||
            !std::isfinite(monster.size.x) || !std::isfinite(monster.size.y) ||
            monster.size.x <= 0 || monster.size.y <= 0) {
            error = "Monster positions must be finite and sizes must be positive";
            return false;
        }
    }
    return true;
}
} // namespace

Monsters::~Monsters() { unload(); }
void Monsters::unload() {
    instances_.clear();
    for (auto &entry : entries_)
        if (IsTextureValid(entry.texture))
            UnloadTexture(entry.texture);
    entries_.clear();
}
bool Monsters::replace(const std::vector<MonsterDefinition> &definitions, std::string &error) {
    if (!validate(definitions, error))
        return false;
    std::vector<Entry> replacement;
    replacement.reserve(definitions.size());
    for (const auto &definition : definitions) {
        std::shared_ptr<const teya::animation::AnimationAsset> asset;
        Texture2D texture{};
        if (!definition.animationAssetPath.empty()) {
            auto loaded = teya::animation::loadAnimationAsset(
                teya::core::assets::path(definition.animationAssetPath));
            if (loaded) {
                asset = loaded.asset;
                texture = LoadTexture(teya::core::assets::path(asset->texturePath).string().c_str());
                if (IsTextureValid(texture))
                    SetTextureFilter(texture,
                        teya::animation::raylibTextureFilter(asset->render.textureFilter));
                else
                    asset.reset();
            }
        }
        teya::animation::AnimationPlayer player(asset);
        if (asset && !asset->clips.empty())
            (void)player.play(asset->clips.front().name);
        replacement.push_back({definition, std::move(asset), std::move(player), texture});
    }
    unload();
    entries_ = std::move(replacement);
    return true;
}
void Monsters::setInstances(const std::vector<WorldInstance> &instances) {
    instances_.clear();
    for (const auto &instance : instances) {
        if (instance.kind != WorldInstanceKind::Monster)
            continue;
        const auto master = std::find_if(entries_.begin(), entries_.end(), [&](const auto &entry) {
            return entry.definition.id == instance.masterId;
        });
        if (master == entries_.end())
            continue;
        teya::animation::AnimationPlayer player(master->animationAsset);
        if (master->animationAsset && !master->animationAsset->clips.empty())
            (void)player.play(master->animationAsset->clips.front().name);
        instances_.push_back({instance, std::move(player)});
    }
}
bool Monsters::load(const std::filesystem::path &path, std::string &error) {
    try {
        std::ifstream input(path);
        if (!input) { error = "Could not open monster asset: " + path.string(); return false; }
        const auto root = json::parse(input);
        if (root.value("schemaVersion", 0) != 1) { error = "Unsupported monster schema"; return false; }
        std::vector<MonsterDefinition> definitions;
        for (const auto &entry : root.at("monsters")) {
            MonsterDefinition monster;
            monster.id = entry.at("id").get<std::uint64_t>();
            monster.name = entry.value("name", "Monster");
            monster.animationAssetPath = entry.value("animation", "");
            const auto &position = entry.at("position");
            const auto &size = entry.at("size");
            monster.position = {position.at("x").get<float>(), position.at("y").get<float>()};
            monster.size = {size.at("x").get<float>(), size.at("y").get<float>()};
            if (const auto color = entry.find("tint"); color != entry.end())
                monster.tint = {static_cast<unsigned char>(color->value("r", 220)),
                                static_cast<unsigned char>(color->value("g", 80)),
                                static_cast<unsigned char>(color->value("b", 80)), 255};
            definitions.push_back(std::move(monster));
        }
        return replace(definitions, error);
    } catch (const std::exception &exception) { error = exception.what(); return false; }
}
bool Monsters::save(const std::filesystem::path &path, std::string &error) const {
    const auto values = definitions();
    if (!validate(values, error)) return false;
    json root = {{"schemaVersion", 1}, {"monsters", json::array()}};
    for (const auto &monster : values)
        root["monsters"].push_back({{"id", monster.id}, {"name", monster.name},
            {"animation", monster.animationAssetPath},
            {"position", {{"x", monster.position.x}, {"y", monster.position.y}}},
            {"size", {{"x", monster.size.x}, {"y", monster.size.y}}},
            {"tint", {{"r", monster.tint.r}, {"g", monster.tint.g}, {"b", monster.tint.b}}}});
    std::filesystem::create_directories(path.parent_path());
    std::ofstream output(path, std::ios::trunc);
    if (!output) { error = "Could not write monster asset"; return false; }
    output << root.dump(4) << '\n';
    return static_cast<bool>(output);
}
void Monsters::update(float deltaTime) {
    for (auto &instance : instances_)
        instance.animation.update(deltaTime);
}
void Monsters::draw() const {
    for (const auto &instance : instances_) {
        const auto master = std::find_if(entries_.begin(), entries_.end(), [&](const auto &entry) {
            return entry.definition.id == instance.definition.masterId;
        });
        if (master == entries_.end())
            continue;
        auto m = master->definition;
        m.position = instance.definition.position;
        const Rectangle destination{m.position.x - m.size.x * .5f,
                                    m.position.y - m.size.y, m.size.x, m.size.y};
        const auto *frame = instance.animation.currentFrame();
        const auto source = frame && master->animationAsset
                                ? teya::animation::animationSourceRectangle(*master->animationAsset,
                                                                            *frame)
                                : std::nullopt;
        if (master->animationAsset) {
            const auto &shadow = master->animationAsset->render.groundShadow;
            if (shadow.enabled && shadow.color.a > 0) {
                const Vector2 center{m.position.x + shadow.offset.x,
                                     m.position.y + shadow.offset.y};
                Color edge = shadow.color;
                edge.a = static_cast<unsigned char>(edge.a * .28f);
                DrawEllipse(static_cast<int>(center.x), static_cast<int>(center.y),
                            shadow.size.x * .58f, shadow.size.y * .68f, edge);
                DrawEllipse(static_cast<int>(center.x), static_cast<int>(center.y),
                            shadow.size.x * .5f, shadow.size.y * .5f, shadow.color);
            }
        }
        if (source && IsTextureValid(master->texture)) {
            Rectangle sourceBounds = *source;
            const auto *clip = master->animationAsset->findClip(instance.animation.currentClipName());
            if (clip && clip->mirrored)
                sourceBounds.width = -sourceBounds.width;
            DrawTexturePro(master->texture, sourceBounds, destination, {}, 0, WHITE);
        } else {
            constexpr Color MissingAssetPink{255, 0, 255, 255};
            DrawRectangleRec(destination, MissingAssetPink);
            DrawRectangleLinesEx(destination, 1, Color{90, 20, 20, 255});
        }
    }
}
std::vector<MonsterDefinition> Monsters::definitions() const {
    std::vector<MonsterDefinition> result;
    result.reserve(entries_.size());
    for (const auto &entry : entries_) result.push_back(entry.definition);
    return result;
}
