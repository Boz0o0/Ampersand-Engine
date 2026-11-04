#pragma once

#include <array>
#include <cstdint>
#include <filesystem>
#include <map>
#include <nlohmann/json.hpp>
#include <optional>
#include <string>
#include <string_view>
#include <vector>

/**
 * @file Project.hpp
 * @brief Project opened in the editor and its JSON documents.
 */

namespace ampersand::editor {

/** @brief Path to a value of a JSON document. */
using Pointer = nlohmann::ordered_json::json_pointer;

/**
 * @brief JSON file of the project, kept in memory until the next build.
 */
struct Document {
    std::filesystem::path path;   // Relative to the project root
    nlohmann::ordered_json data;  // Ordered: keeps the key order of the file
    bool dirty{false};            // Changed since the last build
};

/**
 * @brief File or folder of the project, as listed by the last refresh.
 */
struct FileNode {
    std::filesystem::path path;  // Relative to the project root
    std::string name;            // File name, shown in the Files panel
    bool directory{false};
    std::vector<FileNode> children;  // Folders first, then sorted by name
};

/**
 * @brief Scene file opened in the editor.
 */
struct Scene {
    std::string name;  // "name" of the file, or its stem if it has none
    Document document;
};

/**
 * @brief Game folder opened in the editor (assets/, scripts/ and world/).
 */
class Project {
public:
    /** @brief Path of the configuration file, relative to the root. */
    static constexpr std::string_view kConfigPath{"world/config.json"};

    /**
     * @brief Opens the game folder at root.
     *
     * @return The project, or std::nullopt if root has no readable
     * world/config.json holding a JSON object.
     */
    static std::optional<Project> open(const std::filesystem::path& root);

    /**
     * @brief Creates a game folder at root.
     *
     * @return The project, or std::nullopt if root has no readable
     * world/config.json holding a JSON object.
     */
    static std::optional<Project> create(const std::filesystem::path& root,
                                         const std::string& name);

    /**
     * @brief Whether name can be used as a single file or folder name: not
     * empty, not "." or "..", and without separator or control character.
     */
    static bool isValidName(std::string_view name);

    /**
     * @brief Creates an empty scene at world/scenes/<name>.json.
     *
     * @return std::nullopt on success, otherwise the error message.
     */
    std::optional<std::string> createScene(const std::string& name);

    /**
     * @brief Saves all dirty documents to disk.
     *
     * @return std::nullopt if all documents were saved successfully, otherwise
     * the error message.
     */
    std::optional<std::string> build();

    /** @brief Absolute path of the game folder. */
    [[nodiscard]] const std::filesystem::path& getRoot() const { return _root; }

    /** @brief Name of the game folder. */
    [[nodiscard]] std::string getProjectName() const {
        return _root.filename().string();
    }

    /** @brief The world/config.json document. */
    [[nodiscard]] Document& getConfig() { return _config; }

    /** @brief The assets/, scripts/ and world/ folders that exist. */
    [[nodiscard]] const std::vector<FileNode>& getFiles() const {
        return _files;
    }

    /** @brief The scene opened, if any. */
    [[nodiscard]] std::optional<Scene>& getScene() { return _scene; }

    /** @brief Whether the opened scene changed since the last build. */
    [[nodiscard]] bool isSceneDirty() const {
        return _scene && _scene->document.dirty;
    }

    /** @brief Whether a document changed since the last build. */
    [[nodiscard]] bool isDirty() const {
        return _config.dirty || isSceneDirty();
    }

    /** @brief Lists the project folders again from the disk. */
    void refreshFiles();

    /**
     * @brief Opens the scene file at relative, replacing the opened one.
     *
     * @return false, keeping the opened scene, if the file is not a scene.
     */
    bool openScene(const std::filesystem::path& relative);

    /**
     * @brief The prefab file at relative, read once then kept in memory.
     * The editor only reads prefabs, it never changes them.
     *
     * @return The prefab, or null if the file can't be read.
     */
    const nlohmann::ordered_json& getPrefab(const std::string& relative);

    /**
     * @brief entity with its prefab applied, as SceneManager::resolvePrefab
     * does: the components of the entity merge-patched over the ones of its
     * prefab (itself resolved), and the children of both, resolved.
     *
     * @throw std::runtime_error If a prefab can't be read or uses itself.
     */
    nlohmann::ordered_json resolved(const nlohmann::ordered_json& entity);

    /**
     * @brief The smallest merge patch turning base into target: keys whose
     * values differ, recursing into objects, and null for the keys of base
     * that target lacks. Not an RFC 6902 patch like nlohmann's diff.
     */
    static nlohmann::ordered_json mergeDiff(
        const nlohmann::ordered_json& base,
        const nlohmann::ordered_json& target);

    /*
     * Edition of the opened scene. An entity is given by its pointer in the
     * scene file, a parent by its pointer too or by an empty one for the root.
     * Each returns false, changing nothing, if a pointer is not valid, and
     * marks the scene dirty otherwise.
     */

    /** @brief Adds an empty entity, with a default Transform, to parent. */
    bool createEntity(const Pointer& parent);

    /** @brief Inserts a copy of entity right after it. */
    bool duplicateEntity(const Pointer& entity);

    /** @brief Removes entity and its children. */
    bool deleteEntity(const Pointer& entity);

    /**
     * @brief Moves entity at the end of the children of parent.
     * Fails if parent is entity itself, one of its children or its parent.
     */
    bool moveEntity(const Pointer& entity, const Pointer& parent);

    /** @brief Sets the name of entity. */
    bool renameEntity(const Pointer& entity, const std::string& name);

    /** @brief Whether pointer is an entity of the scene data. */
    static bool isEntity(const nlohmann::ordered_json& data,
                         const Pointer& pointer);

    /** @brief Whether pointer is root or one of the values inside it. */
    static bool isInside(const Pointer& pointer, const Pointer& root);

private:
    /**
     * @brief Recursion of resolved. chain holds the prefabs being resolved,
     * to detect a prefab using itself.
     */
    nlohmann::ordered_json resolve(const nlohmann::ordered_json& description,
                                   std::vector<std::string>& chain);

    /** @brief Version of the scene files the engine loads. */
    static constexpr std::int32_t kSceneVersion{2};

    /**
     * @brief Checks the file has the shape of a scene, not of a prefab.
     */
    static bool isScene(const nlohmann::ordered_json& data);

    /** @brief Pointer to the children array of parent (root if empty). */
    static Pointer childrenPointer(const Pointer& parent);

    /**
     * @brief The children array of parent in the scene data, created if
     * missing. parent must be empty or an entity of data.
     */
    static nlohmann::ordered_json& childrenOf(nlohmann::ordered_json& data,
                                              const Pointer& parent);

    /** @brief Folders shown in the Files panel, relative to the root. */
    static constexpr std::array<std::string_view, 3> kFolders{
        "assets", "scripts", "world"};

    /** @brief Folder of the scenes, relative to the root. */
    static constexpr std::string_view kSceneFolder{"world/scenes"};

    /** @brief Data of an empty scene named name. */
    static nlohmann::ordered_json emptyScene(const std::string& name);

    /** @brief Reads a JSON object, or std::nullopt if it can't. */
    static std::optional<nlohmann::ordered_json> readJson(
        const std::filesystem::path& path);

    /** @brief Lists the folder at root / relative, recursively. */
    static std::vector<FileNode> listDirectory(
        const std::filesystem::path& root,
        const std::filesystem::path& relative);

    /** @brief Writes document to root / document.path if dirty, then clears
     * dirty. */
    static std::optional<std::string> writeDocument(
        const std::filesystem::path& root, Document& document);

    std::filesystem::path _root;
    Document _config;
    std::vector<FileNode> _files;
    std::optional<Scene> _scene;
    std::map<std::string, nlohmann::ordered_json> _prefabs;  // By path
};

}  // namespace ampersand::editor
