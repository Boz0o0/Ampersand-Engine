#pragma once

#include <filesystem>
#include <nlohmann/json.hpp>
#include <optional>
#include <string>
#include <string_view>

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

private:
    /** @brief Reads a JSON object, or std::nullopt if it can't. */
    static std::optional<nlohmann::ordered_json> readJson(
        const std::filesystem::path& path);

    std::filesystem::path _root;
    Document _config;
};

}  // namespace ampersand::editor
