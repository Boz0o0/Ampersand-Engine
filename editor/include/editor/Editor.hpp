#pragma once

#include <SFML/Graphics/RenderWindow.hpp>
#include <SFML/System/Clock.hpp>
#include <editor/Project.hpp>
#include <optional>
#include <string>

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

    /** @brief Asks for a project folder with the native dialog and opens it. */
    void pickProject();

    /** @brief Handle window events. */
    void pollEvents();

    /** @brief Handle the keyboard shortcuts of the menu. */
    void handleShortcuts();

    /** @brief Draw the error popup while there is an error to show. */
    void drawErrorPopup();

    /** @brief Draw and create the dockspace. */
    static void drawDockspace();

    /** @brief Draw the menu bar. */
    void drawMenuBar();

    /** @brief Draw the files panel. */
    static void drawFiles();

    /** @brief Draw the hierarchy panel. */
    static void drawHierarchy();

    /** @brief Draw the viewport panel. */
    static void drawViewport();

    /** @brief Draw the inspector panel. */
    static void drawInspector();

    std::optional<Project> _project;
    std::string _iniPath;
    std::string _error;
    sf::RenderWindow _window;
    sf::Clock _clock;
};

}  // namespace ampersand::editor
