#include "game/GameEditorHost.h"
#if TEYA_ENABLE_EDITOR
#include "game/Game.h"
#include "player/Player.h"
#include "world/Monsters.h"
#include <algorithm>
#include <cctype>
#include <filesystem>
#include <memory>
#include <optional>
#include <teya/animation/AnimationIO.h>
#include <teya/core/AssetPath.h>
#include <teya/core/Log.h>
#include <teya/core/Profile.h>
namespace {
std::filesystem::path animationDirectory() { return teya::core::assets::path("animations"); }
std::string relativePath(const std::filesystem::path &p) {
    return "animations/" + p.filename().generic_string();
}
std::uint64_t animationId(std::string_view path) {
    std::uint64_t h = 14695981039346656037ull;
    for (unsigned char c : path) {
        h ^= c;
        h *= 1099511628211ull;
    }
    return h;
}
bool isAnimationFile(const std::filesystem::path &p) {
    const auto n = p.filename().string();
    constexpr std::string_view s = ".animation.json";
    return n.size() > s.size() && n.compare(n.size() - s.size(), s.size(), s) == 0;
}
std::string displayName(const std::filesystem::path &p) {
    auto n = p.filename().string();
    constexpr std::string_view s = ".animation.json";
    n.resize(n.size() - s.size());
    return n;
}
std::optional<std::filesystem::path> pathForId(std::uint64_t id) {
    std::error_code ec;
    for (std::filesystem::directory_iterator it(animationDirectory(), ec), end; !ec && it != end;
         it.increment(ec))
        if (it->is_regular_file(ec) && isAnimationFile(it->path()) &&
            animationId(relativePath(it->path())) == id)
            return it->path();
    return std::nullopt;
}
std::optional<std::filesystem::path> destinationForName(std::string_view requested,
                                                        std::string &error) {
    std::string name(requested);
    constexpr std::string_view suffix = ".animation.json";
    if (name.size() >= suffix.size() &&
        name.compare(name.size() - suffix.size(), suffix.size(), suffix) == 0)
        name.resize(name.size() - suffix.size());
    if (name.empty()) {
        error = "Name cannot be empty";
        return std::nullopt;
    }
    if (!std::all_of(name.begin(), name.end(),
                     [](unsigned char c) { return std::isalnum(c) || c == '_' || c == '-'; })) {
        error = "Use only letters, numbers, underscores, and hyphens";
        return std::nullopt;
    }
    auto path = animationDirectory() / (name + std::string(suffix));
    if (std::filesystem::exists(path)) {
        error = "An animation with that name already exists";
        return std::nullopt;
    }
    return path;
}
} // namespace
GameEditorHost::GameEditorHost(Game &game) : game_(game) {
    savedPlayerCollider_ = game_.editorPlayer().colliderConfig();
}
GameEditorHost::~GameEditorHost() {
    for (auto &entry : previewTextures_)
        if (IsTextureValid(entry.second))
            UnloadTexture(entry.second);
    for (auto &entry : attachmentTextures_)
        if (IsTextureValid(entry.second))
            UnloadTexture(entry.second);
}
RenderTexture2D GameEditorHost::gameViewTexture() const { return game_.editorTexture(); }
int GameEditorHost::gameCanvasWidth() const { return game_.editorCanvasWidth(); }
int GameEditorHost::gameCanvasHeight() const { return game_.editorCanvasHeight(); }
std::vector<teya::editor::RuntimeNode> GameEditorHost::runtimeHierarchy() const {
    return game_.editorHierarchy();
}
std::vector<teya::editor::RuntimeProperty>
GameEditorHost::inspectObject(teya::editor::RuntimeObjectId id) const {
    return game_.editorProperties(id);
}
teya::editor::EditorFrameMetrics GameEditorHost::frameMetrics() const {
    return game_.editorMetrics();
}
void GameEditorHost::setDebugDrawSettings(
    const teya::editor::EditorDebugDrawSettings &settings) {
    game_.setEditorDebugDrawSettings(settings);
}
std::optional<teya::editor::EditableColliderInfo>
GameEditorHost::editableCollider(teya::editor::RuntimeObjectId id) const {
    constexpr teya::editor::RuntimeObjectId PlayerObjectId = 5;
    if (id != PlayerObjectId)
        return std::nullopt;
    const auto &player = game_.editorPlayer();
    const auto &config = player.colliderConfig();
    const bool saved = config.offset.x == savedPlayerCollider_.offset.x &&
                       config.offset.y == savedPlayerCollider_.offset.y &&
                       config.size.x == savedPlayerCollider_.size.x &&
                       config.size.y == savedPlayerCollider_.size.y &&
                       config.shadowVisible == savedPlayerCollider_.shadowVisible &&
                       config.shadowOffset.x == savedPlayerCollider_.shadowOffset.x &&
                       config.shadowOffset.y == savedPlayerCollider_.shadowOffset.y &&
                       config.shadowSize.x == savedPlayerCollider_.shadowSize.x &&
                       config.shadowSize.y == savedPlayerCollider_.shadowSize.y &&
                       config.shadowColor.r == savedPlayerCollider_.shadowColor.r &&
                       config.shadowColor.g == savedPlayerCollider_.shadowColor.g &&
                       config.shadowColor.b == savedPlayerCollider_.shadowColor.b &&
                       config.shadowColor.a == savedPlayerCollider_.shadowColor.a;
    return teya::editor::EditableColliderInfo{id, "Player", player.position(), config.offset,
                                               config.size, saved};
}
teya::editor::ColliderEditResult
GameEditorHost::applyEditableCollider(teya::editor::RuntimeObjectId id, Vector2 offset,
                                      Vector2 size) {
    constexpr teya::editor::RuntimeObjectId PlayerObjectId = 5;
    if (id != PlayerObjectId)
        return {false, "This object has no editable collider"};
    std::string error;
    if (!game_.editorPlayer().applyColliderConfig({offset, size}, error))
        return {false, error};
    return {true, {}};
}
teya::editor::ColliderEditResult
GameEditorHost::saveEditableCollider(teya::editor::RuntimeObjectId id) {
    constexpr teya::editor::RuntimeObjectId PlayerObjectId = 5;
    if (id != PlayerObjectId)
        return {false, "This object has no editable collider"};
    const auto config = game_.editorPlayer().colliderConfig();
    std::string error;
    if (!savePlayerColliderConfig(teya::core::assets::path("player/player.config.json"), config,
                                  error))
        return {false, error};
    savedPlayerCollider_ = config;
    return {true, {}};
}
teya::editor::ColliderEditResult
GameEditorHost::reloadEditableCollider(teya::editor::RuntimeObjectId id) {
    constexpr teya::editor::RuntimeObjectId PlayerObjectId = 5;
    if (id != PlayerObjectId)
        return {false, "This object has no editable collider"};
    PlayerColliderConfig config;
    std::string error;
    if (!loadPlayerColliderConfig(teya::core::assets::path("player/player.config.json"), config,
                                  error))
        return {false, error};
    if (!game_.editorPlayer().applyColliderConfig(config, error))
        return {false, error};
    savedPlayerCollider_ = config;
    return {true, {}};
}
std::optional<teya::editor::EditableGroundShadowInfo>
GameEditorHost::editableGroundShadow(teya::editor::RuntimeObjectId id) const {
    constexpr teya::editor::RuntimeObjectId PlayerObjectId = 5;
    if (id != PlayerObjectId)
        return std::nullopt;
    const auto &config = game_.editorPlayer().colliderConfig();
    return teya::editor::EditableGroundShadowInfo{id, config.shadowVisible,
                                                   config.shadowOffset, config.shadowSize,
                                                   config.shadowColor};
}
teya::editor::ColliderEditResult GameEditorHost::applyEditableGroundShadow(
    teya::editor::RuntimeObjectId id, bool visible, Vector2 offset, Vector2 size, Color color) {
    constexpr teya::editor::RuntimeObjectId PlayerObjectId = 5;
    if (id != PlayerObjectId)
        return {false, "This object has no editable ground shadow"};
    auto config = game_.editorPlayer().colliderConfig();
    config.shadowVisible = visible;
    config.shadowOffset = offset;
    config.shadowSize = size;
    config.shadowColor = color;
    std::string error;
    if (!game_.editorPlayer().applyColliderConfig(config, error))
        return {false, error};
    return {true, {}};
}
teya::editor::MonsterWorkingCopyResult GameEditorHost::loadEditableMonsters() {
    Monsters temporary;
    std::string error;
    if (!temporary.load(teya::core::assets::path("monsters/monsters.json"), error))
        return {false, {}, error};
    std::vector<teya::editor::EditableMonster> result;
    for (const auto &monster : temporary.definitions())
        result.push_back({monster.id, monster.name, monster.animationAssetPath, monster.position,
                          monster.size, monster.tint, monster.moveSpeed, monster.maxHealth,
                          monster.stopDistance, monster.attackRange, monster.attackCooldown,
                          monster.attackDamage, monster.separationRadius,
                          monster.separationStrength, monster.surroundRadius});
    return {true, std::move(result), {}};
}
teya::editor::ColliderEditResult GameEditorHost::saveAndApplyEditableMonsters(
    const std::vector<teya::editor::EditableMonster> &editable) {
    std::vector<MonsterDefinition> definitions;
    definitions.reserve(editable.size());
    for (const auto &monster : editable)
        definitions.push_back({monster.id, monster.name, monster.animationAssetPath,
                               monster.position, monster.size, monster.tint, monster.moveSpeed,
                               monster.maxHealth, monster.stopDistance, monster.attackRange,
                               monster.attackCooldown, monster.attackDamage,
                               monster.separationRadius, monster.separationStrength,
                               monster.surroundRadius});
    Monsters validated;
    std::string error;
    if (!validated.replace(definitions, error))
        return {false, error};
    if (!validated.save(teya::core::assets::path("monsters/monsters.json"), error))
        return {false, error};
    if (!game_.editorWorld().replaceMonsters(definitions, error))
        return {false, "Saved, but live application failed: " + error};
    return {true, {}};
}
teya::editor::InstanceWorkingCopyResult GameEditorHost::loadEditableWorldInstances() {
    std::vector<teya::editor::EditableWorldInstance> result;
    for (const auto &instance : game_.editorWorld().editorInstances())
        result.push_back({instance.id,
                          instance.kind == WorldInstanceKind::Player
                              ? teya::editor::EditableInstanceKind::Player
                              : teya::editor::EditableInstanceKind::Monster,
                          instance.masterId, instance.position, instance.name});
    return {true, std::move(result), {}};
}
teya::editor::ColliderEditResult GameEditorHost::saveAndApplyWorldInstances(
    const std::vector<teya::editor::EditableWorldInstance> &editable) {
    std::vector<WorldInstance> instances;
    instances.reserve(editable.size());
    for (const auto &instance : editable)
        instances.push_back({instance.id,
                             instance.kind == teya::editor::EditableInstanceKind::Player
                                 ? WorldInstanceKind::Player : WorldInstanceKind::Monster,
                             instance.masterId, instance.position, instance.name});
    std::string error;
    if (!game_.editorWorld().saveAndApplyInstances(instances, error)) return {false, error};
    game_.requestGameRestart(true);
    return {true, {}};
}
std::vector<teya::editor::CustomGameplayFeature> GameEditorHost::customGameplayFeatures() const {
    using Type = teya::editor::GameplaySettingType;
    const auto &config = game_.editorWorld().herdChallenge().config();
    teya::editor::CustomGameplayFeature feature;
    feature.id = 1; feature.name = "Herd Challenge"; feature.enabled = config.enabled;
    feature.settings = {
        {"standstill_seconds", "Standstill before swing", Type::Float, false, 0,
         config.standstillSeconds, .1f, 10.0f, .1f},
        {"starting_slimes", "Starting slimes", Type::Integer, false,
         config.startingSlimes, 0, 1, 100, 1},
        {"slimes_per_level", "Additional slimes per level", Type::Integer, false,
         config.slimesPerLevel, 0, 0, 25, 1},
        {"one_hit_challenge", "One-hit challenge", Type::Boolean,
         config.oneHitChallenge, 0, 0, 0, 1, 1},
        {"result_popup_delay", "Result popup delay", Type::Float, false, 0,
         config.resultPopupDelay, 0, 10.0f, .1f}};
    return {std::move(feature)};
}
std::vector<teya::editor::GameplayDiagnostic>
GameEditorHost::customGameplayDiagnostics(std::uint64_t featureId) const {
    if (featureId != 1) return {};
    const auto &challenge = game_.editorWorld().herdChallenge();
    const int nextMonsterCount = challenge.requestedMonsterCount() +
        (challenge.state() == HerdChallengeState::LevelComplete
             ? challenge.config().slimesPerLevel : 0);
    return {{"State", herdChallengeStateName(challenge.state())},
            {"Level", std::to_string(challenge.level())},
            {"Still time", std::to_string(challenge.stillTime()) + " s"},
            {"Slimes hit", std::to_string(challenge.hitCount()) + " / " +
                               std::to_string(challenge.requiredHits())},
            {"Next monster count", std::to_string(nextMonsterCount)}};
}
std::vector<teya::editor::GameplayAction>
GameEditorHost::customGameplayActions(std::uint64_t featureId) const {
    if (featureId != 1) return {};
    const auto state = game_.editorWorld().herdChallenge().state();
    if (state == HerdChallengeState::LevelComplete) return {{"next_level", "Next Level"}};
    if (state == HerdChallengeState::Missed) return {{"retry", "Retry"}};
    return {};
}
teya::editor::ColliderEditResult GameEditorHost::invokeCustomGameplayAction(
    std::uint64_t featureId, std::string_view action) {
    if (featureId != 1) return {false, "Unknown custom gameplay feature"};
    if (action == "next_level")
        return game_.editorWorld().advanceHerdChallenge()
                   ? teya::editor::ColliderEditResult{true, {}}
                   : teya::editor::ColliderEditResult{false, "Next Level is not available"};
    if (action == "retry")
        return game_.editorWorld().retryHerdChallenge()
                   ? teya::editor::ColliderEditResult{true, {}}
                   : teya::editor::ColliderEditResult{false, "Retry is not available"};
    return {false, "Unknown Herd Challenge action"};
}
teya::editor::ColliderEditResult GameEditorHost::saveAndApplyCustomGameplayFeature(
    const teya::editor::CustomGameplayFeature &feature) {
    if (feature.id != 1) return {false, "Unknown custom gameplay feature"};
    auto config = game_.editorWorld().herdChallenge().config();
    config.enabled = feature.enabled;
    for (const auto &setting : feature.settings) {
        if (setting.key == "standstill_seconds") config.standstillSeconds = setting.floatValue;
        else if (setting.key == "starting_slimes") config.startingSlimes = setting.intValue;
        else if (setting.key == "slimes_per_level") config.slimesPerLevel = setting.intValue;
        else if (setting.key == "one_hit_challenge") config.oneHitChallenge = setting.boolValue;
        else if (setting.key == "result_popup_delay")
            config.resultPopupDelay = setting.floatValue;
    }
    std::string error;
    return game_.editorWorld().saveAndApplyHerdChallenge(config, error)
               ? teya::editor::ColliderEditResult{true, {}}
               : teya::editor::ColliderEditResult{false, error};
}
std::vector<teya::editor::EditableAnimationAssetInfo>
GameEditorHost::editableAnimationAssets() const {
    std::vector<teya::editor::EditableAnimationAssetInfo> result;
    std::error_code ec;
    std::filesystem::create_directories(animationDirectory(), ec);
    for (std::filesystem::directory_iterator it(animationDirectory(), ec), end; !ec && it != end;
         it.increment(ec)) {
        if (!it->is_regular_file(ec) || !isAnimationFile(it->path()))
            continue;
        const auto relative = relativePath(it->path());
        auto loaded = teya::animation::loadAnimationAsset(it->path());
        int errors = 0, warnings = 0;
        for (const auto &issue : loaded.validation.issues)
            (issue.severity == teya::animation::AnimationValidationSeverity::Error ? errors
                                                                                   : warnings)++;
        if (!loaded && errors == 0)
            errors = 1;
        result.push_back({animationId(relative), displayName(it->path()), relative,
                          static_cast<bool>(loaded), errors, warnings,
                          relative == game_.editorPlayer().animationAssetPath()});
    }
    return result;
}
teya::editor::AnimationAssetOperationResult
GameEditorHost::createAnimationAsset(std::string_view name) {
    std::string error;
    auto path = destinationForName(name, error);
    if (!path)
        return {false, 0, error};
    teya::animation::AnimationAsset asset;
    asset.texturePath = "textures/player-sheet.png";
    asset.frameWidth = 48;
    asset.frameHeight = 48;
    asset.sheetColumns = 6;
    teya::animation::AnimationClip clip;
    clip.name = "idle";
    clip.frames.push_back({});
    asset.clips.push_back(std::move(clip));
    std::filesystem::create_directories(path->parent_path());
    if (!teya::animation::saveAnimationAsset(*path, asset, nullptr, &error))
        return {false, 0, error};
    const auto relative = relativePath(*path);
    return {true, animationId(relative), {}};
}
teya::editor::AnimationAssetOperationResult
GameEditorHost::duplicateAnimationAsset(std::uint64_t id, std::string_view name) {
    auto source = pathForId(id);
    if (!source)
        return {false, 0, "Animation asset was not found"};
    std::string error;
    auto destination = destinationForName(name, error);
    if (!destination)
        return {false, 0, error};
    std::error_code ec;
    if (!std::filesystem::copy_file(*source, *destination, std::filesystem::copy_options::none,
                                    ec) ||
        ec)
        return {false, 0, "Could not duplicate animation: " + ec.message()};
    const auto relative = relativePath(*destination);
    return {true, animationId(relative), {}};
}
teya::editor::AnimationAssetOperationResult
GameEditorHost::renameAnimationAsset(std::uint64_t id, std::string_view name) {
    auto source = pathForId(id);
    if (!source)
        return {false, 0, "Animation asset was not found"};
    if (relativePath(*source) == game_.editorPlayer().animationAssetPath())
        return {false, 0, "The runtime Player asset cannot be renamed"};
    std::string error;
    auto destination = destinationForName(name, error);
    if (!destination)
        return {false, 0, error};
    std::error_code ec;
    std::filesystem::rename(*source, *destination, ec);
    if (ec)
        return {false, 0, "Could not rename animation: " + ec.message()};
    if (auto cached = previewTextures_.find(id); cached != previewTextures_.end()) {
        if (IsTextureValid(cached->second))
            UnloadTexture(cached->second);
        previewTextures_.erase(cached);
        previewTexturePaths_.erase(id);
    }
    const auto relative = relativePath(*destination);
    return {true, animationId(relative), {}};
}
teya::editor::AnimationAssetOperationResult GameEditorHost::deleteAnimationAsset(std::uint64_t id) {
    auto path = pathForId(id);
    if (!path)
        return {false, 0, "Animation asset was not found"};
    if (relativePath(*path) == game_.editorPlayer().animationAssetPath())
        return {false, 0, "The runtime Player asset cannot be deleted"};
    std::error_code ec;
    if (!std::filesystem::remove(*path, ec) || ec)
        return {false, 0, "Could not delete animation: " + ec.message()};
    if (auto cached = previewTextures_.find(id); cached != previewTextures_.end()) {
        if (IsTextureValid(cached->second))
            UnloadTexture(cached->second);
        previewTextures_.erase(cached);
        previewTexturePaths_.erase(id);
    }
    return {true, 0, {}};
}
teya::editor::AnimationWorkingCopyResult
GameEditorHost::loadAnimationWorkingCopy(std::uint64_t id) {
    auto path = pathForId(id);
    if (!path) {
        teya::editor::AnimationWorkingCopyResult r;
        r.error = "Unknown animation asset";
        return r;
    }
    auto &p = game_.editorPlayer();
    if (relativePath(*path) == p.animationAssetPath() && p.animationAsset()) {
        auto texture = p.animationTexture();
        return {p.animationAsset(), texture, texture.width, texture.height, {}};
    }
    auto loaded = teya::animation::loadAnimationAsset(*path);
    if (!loaded) {
        teya::editor::AnimationWorkingCopyResult r;
        r.error = loaded.error;
        return r;
    }
    auto found = previewTextures_.find(id);
    const auto cachedPath = previewTexturePaths_.find(id);
    if (found != previewTextures_.end() && (cachedPath == previewTexturePaths_.end() ||
                                            cachedPath->second != loaded.asset->texturePath)) {
        if (IsTextureValid(found->second))
            UnloadTexture(found->second);
        previewTextures_.erase(found);
        found = previewTextures_.end();
    }
    if (found == previewTextures_.end()) {
        auto texture =
            LoadTexture(teya::core::assets::path(loaded.asset->texturePath).string().c_str());
        found = previewTextures_.emplace(id, texture).first;
        previewTexturePaths_[id] = loaded.asset->texturePath;
    }
    auto texture = found->second;
    return {loaded.asset, texture, texture.width, texture.height, {}};
}
teya::animation::AnimationValidationResult
GameEditorHost::validateEditableAnimation(const teya::animation::AnimationAsset &a, int w,
                                          int h) const {
    return teya::animation::validateAnimationAsset(a, w, h);
}
teya::editor::AnimationSaveResult
GameEditorHost::saveAndApplyAnimationAsset(std::uint64_t id,
                                           const teya::animation::AnimationAsset &a) {
    TEYA_PROFILE_ZONE_NAMED("AnimationEditorHost::saveAndApply");
    auto path = pathForId(id);
    if (!path)
        return {false, false, "Unknown animation asset"};
    Texture2D texture{};
    const bool runtime = relativePath(*path) == game_.editorPlayer().animationAssetPath();
    if (runtime)
        texture = game_.editorPlayer().animationTexture();
    else if (auto found = previewTextures_.find(id); found != previewTextures_.end())
        texture = found->second;
    auto validation = validateEditableAnimation(a, texture.width, texture.height);
    if (!validation.valid())
        return {false, false, "Animation validation failed"};
    std::string error;
    if (!teya::animation::saveAnimationAsset(*path, a, nullptr, &error))
        return {false, false, error};
    if (!runtime) {
        if (!game_.editorWorld().reloadMonsters(error))
            return {true, false, "Saved animation, but monster reload failed: " + error};
        return {true, true, {}};
    }
    if (!game_.editorPlayer().applyAnimationAsset(
            std::make_shared<const teya::animation::AnimationAsset>(a), error))
        return {true, false, "Saved to disk, but live application failed: " + error};
    return {true, true, {}};
}
teya::editor::AnimationSaveResult
GameEditorHost::applyAnimationAssetWithoutSaving(std::uint64_t id,
                                                 const teya::animation::AnimationAsset &a) {
    TEYA_PROFILE_ZONE_NAMED("AnimationEditorHost::apply");
    auto path = pathForId(id);
    if (!path || relativePath(*path) != game_.editorPlayer().animationAssetPath())
        return {false, false, "Temporary apply is only available for the Player asset"};
    auto validation = validateEditableAnimation(a, game_.editorPlayer().animationTexture().width,
                                                game_.editorPlayer().animationTexture().height);
    if (!validation.valid())
        return {false, false, "Animation validation failed"};
    std::string error;
    if (!game_.editorPlayer().applyAnimationAsset(
            std::make_shared<const teya::animation::AnimationAsset>(a), error))
        return {false, false, error};
    return {false, true, {}};
}
std::vector<teya::editor::AttachmentPreviewInfo>
GameEditorHost::attachmentPreviews(std::uint64_t) const {
    std::vector<teya::editor::AttachmentPreviewInfo> result;
    for (const auto &object : game_.editorPlayer().attachmentObjects()) {
        auto texture = attachmentTextures_.find(object.id);
        auto cachedPath = attachmentTexturePaths_.find(object.id);
        if (texture != attachmentTextures_.end() &&
            (cachedPath == attachmentTexturePaths_.end() || cachedPath->second != object.texturePath)) {
            if (IsTextureValid(texture->second))
                UnloadTexture(texture->second);
            attachmentTextures_.erase(texture);
            texture = attachmentTextures_.end();
        }
        if (texture == attachmentTextures_.end()) {
            auto loaded = LoadTexture(teya::core::assets::path(object.texturePath).string().c_str());
            texture = attachmentTextures_.emplace(object.id, loaded).first;
            attachmentTexturePaths_[object.id] = object.texturePath;
        }
        if (IsTextureValid(texture->second))
            SetTextureFilter(texture->second, object.smoothRotationFiltering
                                                  ? TEXTURE_FILTER_BILINEAR
                                                  : TEXTURE_FILTER_POINT);
        result.push_back({object.id,
                          object.name,
                          object.socketName,
                          object.texturePath,
                          texture->second,
                          object.pivot,
                          object.effectTip,
                          object.positionOffset,
                          object.rotationOffsetDegrees,
                          object.scale,
                          object.layer,
                          object.visible,
                          object.smoothRotationFiltering,
                          object.trailEnabled,
                          object.trailLifetimeSeconds,
                          object.trailWidth,
                          object.trailOpacity,
                          object.trailColor,
                          object.trailSmoothing,
                          false});
    }
    return result;
}
teya::editor::AttachmentObjectSaveResult GameEditorHost::saveAttachmentObjects(
    std::uint64_t,
    const std::vector<teya::editor::AttachmentPreviewInfo> &editorObjects) {
    std::vector<AttachmentObject> objects;
    objects.reserve(editorObjects.size());
    for (const auto &entry : editorObjects) {
        const auto texturePath = teya::core::assets::path(entry.texturePath);
        if (!std::filesystem::is_regular_file(texturePath))
            return {false, "Attachment texture does not exist: " + entry.texturePath};
        objects.push_back({entry.id,
                           entry.name,
                           entry.texturePath,
                           entry.socketName,
                           entry.pivot,
                           entry.effectTip,
                           entry.positionOffset,
                           entry.rotationOffsetDegrees,
                           entry.scale,
                           entry.layer,
                           entry.visible,
                           entry.smoothRotationFiltering,
                           entry.trailEnabled,
                           entry.trailLifetimeSeconds,
                           entry.trailWidth,
                           entry.trailOpacity,
                           entry.trailColor,
                           entry.trailSmoothing});
    }
    if (objects.empty())
        return {false, "Keep at least one attachment object"};
    std::string error;
    const auto path = teya::core::assets::path("attachments/player.attachments.json");
    if (!::saveAttachmentObjects(path, objects, error))
        return {false, error};
    if (!game_.editorPlayer().replaceAttachmentObjects(std::move(objects), error))
        return {false, "Saved, but live application failed: " + error};
    for (auto &entry : attachmentTextures_)
        if (IsTextureValid(entry.second))
            UnloadTexture(entry.second);
    attachmentTextures_.clear();
    attachmentTexturePaths_.clear();
    return {true, {}};
}
std::vector<std::string> GameEditorHost::animationEventSuggestions() const {
    return {"attack_started",  "attack_active", "spawn_slash",
            "attack_finished", "play_sound",    "footstep"};
}
std::vector<std::string> GameEditorHost::animationMarkerTypeSuggestions() const {
    return {"effect", "sound", "footstep", "interaction", "camera"};
}
std::string GameEditorHost::editorLogPath() const {
    return std::filesystem::absolute("logs/teya_game.log").string();
}
void GameEditorHost::flushEditorLog() { teya::core::Log::flush(); }
void GameEditorHost::requestGameRestart(bool pauseAfterRestart) {
    game_.requestGameRestart(pauseAfterRestart);
}
void GameEditorHost::requestExit() { game_.requestEditorExit(); }
#endif
