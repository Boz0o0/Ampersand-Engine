#pragma once

#include <imgui.h>

#include <SFML/Graphics/RenderWindow.hpp>
#include <SFML/System/Clock.hpp>
#include <array>
#include <cstdint>
#include <editor/Project.hpp>
#include <filesystem>
#include <optional>
#include <string>
#include <string_view>
#include <vector>

/**
 * @file Editor.hpp
 * @brief Editor window and its panels.
 */

namespace ampersand::editor {

/**
 * @brief Item selected in the Hierarchy panel.
 *
 * The pointer is empty for the World (world/config.json), "/resources" for
 * the scene resources, and "/entities/3/children/1" for an entity.
 */
struct Selection {
    std::string scene;  // Name of the scene the item belongs to
    Pointer pointer;

    bool operator==(const Selection&) const = default;
};

/**
 * @brief Change of the Hierarchy asked while drawing it, applied once the
 * tree is drawn so no JSON array changes while ImGui iterates it.
 */
struct HierarchyAction {
    enum class Kind : std::uint8_t {
        CreateEmpty,  // target: parent, empty for the root
        Duplicate,    // source: entity
        Delete,       // source: entity
        Move,         // source: entity, target: new parent, empty for root
        Rename,       // source: entity, opens the rename popup
    };
    Kind kind;
    Pointer source = Pointer();
    Pointer target = Pointer();
};

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
    /** @brief Text colour of the prefab instances in the Hierarchy. */
    static constexpr ImVec4 kPrefabColor{0.45F, 0.70F, 1.0F, 1.0F};

    /** @brief Drag and drop payload of a project file: its relative path. */
    static inline const std::string kAssetPayload{"ASSET_PATH"};

    /** @brief Drag and drop payload of an entity: its pointer, as a string. */
    static inline const std::string kEntityPayload{"ENTITY_POINTER"};

    /** @brief Title of the popup renaming an entity. */
    static inline const std::string kRenamePopup{"Rename entity"};

    /** @brief Minimum height of the drop zone below the Hierarchy tree. */
    static constexpr float kDropZoneHeight{24.0F};

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
     * @brief Draw a selectable leaf node, selecting pointer when clicked.
     */
    void drawSelectable(const std::string& label, const Selection& item);

    /**
     * @brief Draw a scene entity and its children, recursively.
     * pointer is its path in the scene file, its last token its ImGui ID.
     */
    void drawEntity(nlohmann::ordered_json& entity, const Pointer& pointer);

    /** @brief Send text as the drag and drop payload of the given type. */
    static void setPayload(const std::string& type, const std::string& text);

    /** @brief Text of a payload sent by setPayload. */
    static std::string payloadText(const ImGuiPayload& payload);

    /** @brief Flags of a Hierarchy tree node. */
    static ImGuiTreeNodeFlags treeFlags(bool leaf, bool selected);

    /** @brief Draw the context menu of the scene entity at pointer. */
    void drawEntityMenu(const Pointer& pointer);

    /**
     * @brief Make the scene entity at pointer draggable, and a drop target
     * re-parenting the dragged entity under it.
     */
    void drawEntityDragDrop(const Pointer& pointer);

    /**
     * @brief Draw the empty space below the tree: dropping an entity there
     * moves it to the root, right-clicking it opens a Create Empty menu.
     */
    void drawHierarchyBackground();

    /** @brief Apply the pending Hierarchy action, if any. */
    void applyHierarchyAction();

    /** @brief Draw the popup renaming the entity at _renaming. */
    void drawRenamePopup();

    /**
     * @brief Draw the children an entity gets from its prefab, greyed out and
     * not selectable since they are not in the scene file.
     * chain holds the prefabs being drawn, to stop on a prefab using itself.
     */
    void drawPrefabChildren(const nlohmann::ordered_json& entity,
                            std::vector<std::string>& chain);

    /** @brief Draw a read-only entity coming from a prefab. */
    void drawPrefabEntity(const nlohmann::ordered_json& entity,
                          std::size_t index, std::vector<std::string>& chain);

    /** @brief Whether the entity has children once its prefab is resolved. */
    bool hasChildren(const nlohmann::ordered_json& entity,
                     std::vector<std::string> chain);

    /**
     * @brief Label of an entity: its name, its prefab's name, or "Entity N"
     * with N its index among its siblings.
     */
    std::string entityLabel(const nlohmann::ordered_json& entity,
                            std::size_t index);

    /** @brief Draw the viewport panel. */
    static void drawViewport();

    /** @brief Draw the inspector panel. */
    static void drawInspector();

    std::optional<Project> _project;
    std::optional<Selection> _selection;
    std::optional<HierarchyAction> _hierarchyAction;
    Pointer _renaming;
    std::array<char, 256> _renameBuffer{};
    std::string _iniPath;
    std::string _error;
    std::string _info;
    sf::RenderWindow _window;
    sf::Clock _clock;
    bool _running = true;
};

}  // namespace ampersand::editor
