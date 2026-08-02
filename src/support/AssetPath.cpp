#include "support/AssetPath.h"
#include <string>

namespace assets {

std::filesystem::path path(std::string_view relativePath) {
#ifdef CPP_GAME_DEVELOPMENT_ASSET_DIR
    const std::filesystem::path assetRoot{CPP_GAME_DEVELOPMENT_ASSET_DIR};
#else
    const std::filesystem::path assetRoot{"assets"};
#endif
    return assetRoot / std::filesystem::path{std::string{relativePath}};
}

} // namespace assets
