#pragma once

#include <SFML/Graphics/RenderWindow.hpp>
#include <SFML/System/Clock.hpp>
#include <editor/Project.hpp>
#include <filesystem>
#include <optional>
#include <string>
#include <string_view>

/**
 * @file Editor.hpp
 * @brief Editor window and its panels.
 */

namespace ampersand::editor {

/**
 * @brief Manages the Editor main loop.
 */
class Editor {
public:
    Editor();
    explicit Editor(const std::string& projectPath);
    ~Editor();

    Editor(const Editor&) = delete;
    Editor& operator=(const Editor&) = delete;
    Editor(Editor&&) = delete;
    Editor& operator=(Editor&&) = delete;

    /**
     * @brief Run the main GUI loop.
     */
    void run();

private:
    /** @brief Creates the window and imgui settings. */
    void initEditor();

    /** @brief Open a project, or show an error popup if path is not one. */
    void open(const std::string& path);

    /** @brief Saves the layout of the current project and loads the new one. */
    void switchLayout(const Project& project);

    /** @brief Open a scene, or show an error popup if file is not one. */
    void openScene(const std::filesystem::path& relative);

    /** @brief Asks for a project folder with the native dialog and opens it. */
    void pickProject();

    /** @brief Build the project. */
    void buildProject();

    /** @brief Handle window events. */
    void pollEvents();

    /** @brief Handle the keyboard shortcuts of the menu. */
    void handleShortcuts();

    /** @brief Draw the error popup while there is an error to show. */
    void drawErrorPopup();

    /** @brief Draw the info popup while there is an info to show. */
    void drawInfoPopup();

    /** @brief Draw and create the dockspace. */
    static void drawDockspace();

    /** @brief Draw the menu bar. */
    void drawMenuBar();

    /** @brief Draw the File menu. */
    void drawFileMenu();

    /** @brief Draw the Edit menu. */
    static void drawEditMenu();

    /** @brief Draw the Help menu. */
    void drawHelpMenu();

    /** @brief Draw the files panel. */
    void drawFiles();

    /**
     * @brief Draw a file node, and its children if it's an open folder.
     * Double-clicking a file opens it as the scene.
     */
    void drawFileNode(const FileNode& node);

    /** @brief Draw the hierarchy panel of the opened scene. */
    void drawHierarchy();

    /**
     * @brief Draw an entity node, and its children if it's an open parent.
     * index is its position among its siblings, used as ImGui ID.
     */
    static void drawEntityNode(const nlohmann::ordered_json& entity, int index);

    /** @brief Draw the viewport panel. */
    static void drawViewport();

    /** @brief Draw the inspector panel. */
    static void drawInspector();

    std::optional<Project> _project;
    std::string _iniPath;
    std::string _error;
    std::string _info;
    sf::RenderWindow _window;
    sf::Clock _clock;
    bool _running = true;
};

}  // namespace ampersand::editor
