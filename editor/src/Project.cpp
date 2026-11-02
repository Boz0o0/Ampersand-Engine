#include <algorithm>
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
    project.refreshFiles();
    return project;
}

void Project::refreshFiles() {
    this->_files.clear();
    for (const std::string_view folder : kFolders) {
        const std::filesystem::path relative{folder};
        if (std::filesystem::is_directory(this->_root / relative)) {
            this->_files.push_back({
                .path = relative,
                .name = std::string(folder),
                .directory = true,
                .children = listDirectory(this->_root, relative),
            });
        }
    }
}

bool Project::openScene(const std::filesystem::path& relative) {
    auto data = readJson(this->_root / relative);
    if (!data || !isScene(*data)) {
        return false;
    }
    const auto name = data->find("name");
    this->_scene = Scene{
        .name = name != data->end() && name->is_string()
                    ? name->get<std::string>()
                    : relative.stem().string(),
        .document = {.path = relative, .data = std::move(*data)},
    };
    return true;
}

bool Project::isScene(const nlohmann::ordered_json& data) {
    const auto version = data.find("version");
    const auto entities = data.find("entities");
    const auto resources = data.find("resources");
    const bool validResources = resources == data.end() ||
                                resources->is_null() || resources->is_object();
    const bool prefab = data.contains("components") ||
                        data.contains("children") || data.contains("prefab");
    return version != data.end() && version->is_number_integer() &&
           *version == kSceneVersion && entities != data.end() &&
           (entities->is_null() || entities->is_array()) && validResources &&
           !prefab;
}

std::vector<FileNode> Project::listDirectory(
    const std::filesystem::path& root, const std::filesystem::path& relative) {
    std::vector<FileNode> nodes;
    std::error_code error;
    for (const auto& entry :
         std::filesystem::directory_iterator(root / relative, error)) {
        std::string name = entry.path().filename().string();
        FileNode node{
            .path = relative / name,
            .name = std::move(name),
            .directory = entry.is_directory(error),
        };
        if (node.directory) {
            node.children = listDirectory(root, node.path);
        }
        nodes.push_back(std::move(node));
    }
    std::ranges::sort(nodes, [](const FileNode& lhs, const FileNode& rhs) {
        if (lhs.directory != rhs.directory) {
            return lhs.directory;
        }
        return lhs.name < rhs.name;
    });
    return nodes;
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
