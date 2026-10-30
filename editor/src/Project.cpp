#include <editor/Project.hpp>
#include <fstream>
#include <system_error>

namespace ampersand::editor {

std::optional<Project> Project::open(const std::filesystem::path& root) {
    std::error_code error;
    const auto absolute = std::filesystem::canonical(root, error);
    if (error || !std::filesystem::is_directory(absolute)) {
        return std::nullopt;
    }
    auto config = readJson(absolute / kConfigPath);
    if (!config) {
        return std::nullopt;
    }
    Project project;
    project._root = absolute;
    project._config = {.path = kConfigPath, .data = std::move(*config)};
    return project;
}

std::optional<nlohmann::ordered_json> Project::readJson(
    const std::filesystem::path& path) {
    std::ifstream file(path);
    if (!file) {
        return std::nullopt;
    }
    auto data = nlohmann::ordered_json::parse(file, nullptr, false);
    if (data.is_discarded() || !data.is_object()) {
        return std::nullopt;
    }
    return data;
}

}  // namespace ampersand::editor
