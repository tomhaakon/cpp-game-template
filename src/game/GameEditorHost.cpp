#include "game/GameEditorHost.h"
#if TEYA_ENABLE_EDITOR
#include "game/Game.h"
#include "player/Player.h"
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
    if (!runtime)
        return {true, true, {}};
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
