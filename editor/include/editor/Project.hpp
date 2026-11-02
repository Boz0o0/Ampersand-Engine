#pragma once

#include <array>
#include <cstdint>
#include <filesystem>
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

    /** @brief Lists the project folders again from the disk. */
    void refreshFiles();

    /**
     * @brief Opens the scene file at relative, replacing the opened one.
     *
     * @return false, keeping the opened scene, if the file is not a scene.
     */
    bool openScene(const std::filesystem::path& relative);

private:
    /** @brief Version of the scene files the engine loads. */
    static constexpr std::int32_t kSceneVersion{2};

    /**
     * @brief Checks the file has the shape of a scene, not of a prefab.
     */
    static bool isScene(const nlohmann::ordered_json& data);

    /** @brief Folders shown in the Files panel, relative to the root. */
    static constexpr std::array<std::string_view, 3> kFolders{
        "assets", "scripts", "world"};

    /** @brief Reads a JSON object, or std::nullopt if it can't. */
    static std::optional<nlohmann::ordered_json> readJson(
        const std::filesystem::path& path);

    /** @brief Lists the folder at root / relative, recursively. */
    static std::vector<FileNode> listDirectory(
        const std::filesystem::path& root,
        const std::filesystem::path& relative);

    std::filesystem::path _root;
    Document _config;
    std::vector<FileNode> _files;
    std::optional<Scene> _scene;
};

}  // namespace ampersand::editor
