#include "support/JsonFile.h"
#include <fstream>
#include <nlohmann/json.hpp>
#include <stdexcept>
#include <system_error>

namespace json_file {

nlohmann::json load(const std::filesystem::path &path) {
    std::ifstream input{path};
    if (!input) {
        throw std::runtime_error("Could not open JSON file for reading: " + path.string());
    }

    try {
        return nlohmann::json::parse(input);
    } catch (const nlohmann::json::parse_error &error) {
        throw std::runtime_error("Could not parse JSON file '" + path.string() +
                                 "': " + error.what());
    }
}

void save(const std::filesystem::path &path, const nlohmann::json &document) {
    const std::filesystem::path parent = path.parent_path();
    if (!parent.empty()) {
        std::error_code error;
        std::filesystem::create_directories(parent, error);
        if (error) {
            throw std::runtime_error("Could not create directory for JSON file '" + path.string() +
                                     "': " + error.message());
        }
    }

    std::ofstream output{path};
    if (!output) {
        throw std::runtime_error("Could not open JSON file for writing: " + path.string());
    }

    output << document.dump(4) << '\n';
    if (!output) {
        throw std::runtime_error("Could not write JSON file: " + path.string());
    }
}

} // namespace json_file
