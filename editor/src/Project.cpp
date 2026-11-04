#include <algorithm>
#include <editor/Project.hpp>
#include <editor/Types.hpp>
#include <fstream>
#include <stdexcept>
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
    project._root = root;
    project._config = {.path = kConfigPath, .data = std::move(*config)};
    project.refreshFiles();
    return project;
}

std::optional<Project> Project::create(const std::filesystem::path& root,
                                       const std::string& name) {
    if (!isValidName(name)) {
        return std::nullopt;
    }
    const std::filesystem::path folder = root / name;
    std::error_code error;
    constexpr std::array<std::string_view, 4> kCreated{
        "assets",
        "scripts",
        "world/prefabs",
        kSceneFolder,
    };
    for (const std::string_view sub : kCreated) {
        std::filesystem::create_directories(folder / sub, error);
        if (error) {
            return std::nullopt;
        }
    }
    const std::string start = "defaultScene";
    Document config{.path = kConfigPath, .data = {}};
    config.data["version"] = kSceneVersion;
    config.data["gameRoot"] = name + "/";
    config.data["startScene"] = start;
    config.data["scenes"][start] =
        (std::filesystem::path(kSceneFolder) / (start + ".json"))
            .generic_string();
    config.data["resources"] = nlohmann::ordered_json::object();
    Document scene{
        .path = config.data["scenes"][start].get<std::string>(),
        .data = emptyScene(start),
    };
    if (writeDocument(folder, config) || writeDocument(folder, scene)) {
        return std::nullopt;
    }
    return open(folder);
}

bool Project::isValidName(std::string_view name) {
    if (name.empty() || name == "." || name == "..") {
        return false;
    }
    return std::ranges::none_of(name, [](char character) {
        return character == '/' || character == '\\' || character == ':' ||
               static_cast<unsigned char>(character) < ' ';
    });
}

std::optional<std::string> Project::createScene(const std::string& name) {
    if (!isValidName(name)) {
        return "Invalid scene name '" + name + "'.";
    }
    Document scene{
        .path = std::filesystem::path(kSceneFolder) / (name + ".json"),
        .data = emptyScene(name),
    };
    if (std::filesystem::exists(this->_root / scene.path)) {
        return "File already exists: " + (this->_root / scene.path).string();
    }
    auto error = writeDocument(this->_root, scene);
    this->refreshFiles();
    return error;
}

nlohmann::ordered_json Project::emptyScene(const std::string& name) {
    nlohmann::ordered_json scene;
    scene["version"] = kSceneVersion;
    scene["name"] = name;
    scene["resources"] = nlohmann::ordered_json::object();
    scene["entities"] = nlohmann::ordered_json::array();
    return scene;
}

std::optional<std::string> Project::build() {
    std::optional<std::string> error = std::nullopt;
    if (this->_config.dirty) {
        error = writeDocument(this->_root, this->_config);
        if (error) {
            return error;
        }
    }
    if (this->_scene && this->_scene->document.dirty) {
        error = writeDocument(this->_root, this->_scene->document);
        if (error) {
            return error;
        }
    }
    return error;
}

void Project::refreshFiles() {
    this->_files.clear();
    this->_prefabs.clear();
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

const nlohmann::ordered_json& Project::getPrefab(const std::string& relative) {
    const std::string key =
        std::filesystem::path(relative).lexically_normal().generic_string();
    auto found = this->_prefabs.find(key);
    if (found == this->_prefabs.end()) {
        auto data = readJson(this->_root / key);
        found = this->_prefabs
                    .emplace(key, data ? std::move(*data)
                                       : nlohmann::ordered_json(nullptr))
                    .first;
    }
    return found->second;
}

nlohmann::ordered_json Project::resolved(const nlohmann::ordered_json& entity) {
    std::vector<std::string> chain;
    return this->resolve(entity, chain);
}

nlohmann::ordered_json Project::resolve(
    const nlohmann::ordered_json& description,
    std::vector<std::string>& chain) {
    nlohmann::ordered_json result = description;
    const auto prefab = description.find("prefab");
    if (prefab != description.end()) {
        if (!prefab->is_string()) {
            throw std::runtime_error("'prefab' must be a file path");
        }
        const std::string path =
            std::filesystem::path(prefab->get<std::string>())
                .lexically_normal()
                .generic_string();
        if (std::ranges::find(chain, path) != chain.end()) {
            throw std::runtime_error("Prefab '" + path + "' uses itself");
        }
        const nlohmann::ordered_json& data = this->getPrefab(path);
        if (!data.is_object()) {
            throw std::runtime_error("Cannot read prefab '" + path + "'");
        }
        chain.push_back(path);
        result = this->resolve(data, chain);
        chain.pop_back();
        result["components"].merge_patch(
            description.value("components", nlohmann::ordered_json::object()));
    } else {
        result["children"] = nlohmann::ordered_json::array();
    }
    for (const auto& child :
         description.value("children", nlohmann::ordered_json::array())) {
        result["children"].push_back(this->resolve(child, chain));
    }
    return result;
}

nlohmann::ordered_json Project::mergeDiff(
    const nlohmann::ordered_json& base, const nlohmann::ordered_json& target) {
    nlohmann::ordered_json patch = nlohmann::ordered_json::object();
    for (const auto& item : target.items()) {
        const auto found = base.find(item.key());
        if (found != base.end() && found->is_object() &&
            item.value().is_object()) {
            nlohmann::ordered_json inner = mergeDiff(*found, item.value());
            if (!inner.empty()) {
                patch[item.key()] = std::move(inner);
            }
        } else if (found == base.end() || *found != item.value()) {
            patch[item.key()] = item.value();
        }
    }
    for (const auto& item : base.items()) {
        if (!target.contains(item.key())) {
            patch[item.key()] = nullptr;
        }
    }
    return patch;
}

bool Project::createEntity(const Pointer& parent) {
    if (!this->_scene) {
        return false;
    }
    Document& document = this->_scene->document;
    if (!parent.empty() && !isEntity(document.data, parent)) {
        return false;
    }
    nlohmann::ordered_json components = nlohmann::ordered_json::object();
    components["Transform"] =
        TypeTable::get().getComponentDefaults("Transform");
    childrenOf(document.data, parent)
        .push_back({
            {"name", "Entity"},
            {"components", std::move(components)},
        });
    document.dirty = true;
    return true;
}

bool Project::duplicateEntity(const Pointer& entity) {
    if (!this->_scene) {
        return false;
    }
    Document& document = this->_scene->document;
    if (!isEntity(document.data, entity)) {
        return false;
    }
    const Pointer parent = entity.parent_pointer();
    auto& siblings = document.data[parent];
    const auto index = static_cast<std::ptrdiff_t>(std::stoul(entity.back()));
    nlohmann::ordered_json copy = document.data[entity];
    siblings.insert(siblings.begin() + index + 1, std::move(copy));
    document.dirty = true;
    return true;
}

bool Project::deleteEntity(const Pointer& entity) {
    if (!this->_scene) {
        return false;
    }
    Document& document = this->_scene->document;
    if (!isEntity(document.data, entity)) {
        return false;
    }
    const Pointer parent = entity.parent_pointer();
    document.data[parent].erase(std::stoul(entity.back()));
    document.dirty = true;
    return true;
}

bool Project::moveEntity(const Pointer& entity, const Pointer& parent) {
    if (!this->_scene) {
        return false;
    }
    Document& document = this->_scene->document;
    if (!isEntity(document.data, entity) ||
        (!parent.empty() && !isEntity(document.data, parent)) ||
        isInside(parent, entity) ||
        childrenPointer(parent) == entity.parent_pointer()) {
        return false;
    }
    const Pointer oldParent = entity.parent_pointer();
    // Appending first keeps the indices of entity valid until it is erased
    childrenOf(document.data, parent).push_back(document.data[entity]);
    document.data[oldParent].erase(std::stoul(entity.back()));
    document.dirty = true;
    return true;
}

bool Project::renameEntity(const Pointer& entity, const std::string& name) {
    if (!this->_scene) {
        return false;
    }
    Document& document = this->_scene->document;
    if (!isEntity(document.data, entity)) {
        return false;
    }
    document.data[entity]["name"] = name;
    document.dirty = true;
    return true;
}

bool Project::isInside(const Pointer& pointer, const Pointer& root) {
    const std::string path = pointer.to_string();
    const std::string prefix = root.to_string();
    return path.starts_with(prefix) &&
           (path.size() == prefix.size() || path[prefix.size()] == '/');
}

bool Project::isEntity(const nlohmann::ordered_json& data,
                       const Pointer& pointer) {
    if (pointer.empty() || pointer.parent_pointer().empty()) {
        return false;
    }
    const Pointer parent = pointer.parent_pointer();
    const bool inChildren =
        parent.back() == "children" || parent == childrenPointer(Pointer());
    return inChildren && data.contains(pointer) && data.at(parent).is_array() &&
           data.at(pointer).is_object();
}

Pointer Project::childrenPointer(const Pointer& parent) {
    return parent.empty() ? Pointer("/entities") : parent / "children";
}

nlohmann::ordered_json& Project::childrenOf(nlohmann::ordered_json& data,
                                            const Pointer& parent) {
    const Pointer pointer = childrenPointer(parent);
    auto& children = data[pointer];
    if (!children.is_array()) {
        children = nlohmann::ordered_json::array();
    }
    return children;
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

std::optional<std::string> Project::writeDocument(
    const std::filesystem::path& root, Document& document) {
    std::ofstream file(root / document.path);
    if (!file) {
        return "Failed to open file " + (root / document.path).string() +
               " for writing.";
    }
    file << document.data.dump(4) << '\n';
    if (!file) {
        return "Failed to write file " + (root / document.path).string() + ".";
    }
    document.dirty = false;
    return std::nullopt;
}

}  // namespace ampersand::editor
