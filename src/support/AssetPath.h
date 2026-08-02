#pragma once
#include <filesystem>
#include <string_view>

namespace assets {

std::filesystem::path path(std::string_view relativePath);

} // namespace assets
