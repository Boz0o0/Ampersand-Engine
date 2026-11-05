#pragma once

#include <imgui.h>

#include <SFML/Graphics/RenderWindow.hpp>
#include <SFML/System/Clock.hpp>
#include <array>
#include <cstdint>
#include <editor/Project.hpp>
#include <editor/Types.hpp>
#include <filesystem>
#include <functional>
#include <optional>
#include <span>
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
 * @brief What the inspector edits in an object of typed values.
 */
struct TypedMapEdit {
    const std::vector<TypeInfo>& types;  // Types offered by the add popup
    std::string addLabel;                // Label of the add button
    bool showConfigOnly{false};          // Offer the configOnly types
    nlohmann::ordered_json prefab;       // Values of the prefab, null if none
    nlohmann::ordered_json overrides;    // Own values of the prefab instance
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

    /** @brief Text colour of the fields overriding their prefab's value. */
    static constexpr ImVec4 kOverrideColor{1.0F, 0.75F, 0.30F, 1.0F};

    /** @brief Text colour of the errors shown in a panel. */
    static constexpr ImVec4 kErrorColor{1.0F, 0.40F, 0.40F, 1.0F};

    /** @brief Drag and drop payload of a project file: its relative path. */
    static inline const std::string kAssetPayload{"ASSET_PATH"};

    /** @brief Drag and drop payload of an entity: its pointer, as a string. */
    static inline const std::string kEntityPayload{"ENTITY_POINTER"};

    /** @brief Title of the popup renaming an entity. */
    static inline const std::string kRenamePopup{"Rename entity"};

    /** @brief Minimum height of the drop zone below the Hierarchy tree. */
    static constexpr float kDropZoneHeight{24.0F};

    /** @brief Size of the text field buffers, final null character included. */
    static constexpr std::size_t kTextSize{1024};

    /** @brief Change of a dragged number per pixel. */
    static constexpr float kDragSpeed{0.1F};

    /*
     * Keys of the JSON objects edited with a single widget instead of a tree.
     */
    static constexpr std::array<std::string_view, 2> kVec2Keys{"x", "y"};
    static constexpr std::array<std::string_view, 3> kVec3Keys{"x", "y", "z"};
    static constexpr std::array<std::string_view, 4> kRectKeys{
        "x", "y", "width", "height"};
    static constexpr std::array<std::string_view, 4> kColorKeys{"r", "g", "b",
                                                                "a"};

    /**
     * @brief Keys of the strings holding a project file path ("scripts" for
     * the items of the array of the Scripts component, "scenes" for the
     * entries of the scenes of the configuration).
     */
    static constexpr std::array<std::string_view, 7> kAssetKeys{
        "texture", "font", "sound", "music", "path", "scripts", "scenes"};

    /** @brief Shown in a choice combo for a null value. */
    static constexpr std::string_view kNoChoice{"(none)"};

    /** @brief Name of a new entry of a map, numbered if already used. */
    static constexpr std::string_view kNewEntry{"New entry"};

    /** @brief Creates the window and imgui settings. */
    void initEditor();

    /** @brief Open a project, or show an error popup if path is not one. */
    void open(const std::string& path);

    /** @brief Create a project, or show an error popup if path is not valid. */
    void create(const std::string& path, const std::string& name);

    /** @brief Saves the layout of the current project and loads the new one. */
    void switchLayout(const Project& project);

    /** @brief Open a scene, or show an error popup if file is not one. */
    void openScene(const std::filesystem::path& relative);

    /** @brief Asks for a project folder with the native dialog and opens it. */
    void pickProject(bool isCreating = false);

    /** @brief Create a new scene. */
    void createNewScene(const std::string& name);

    /** @brief Build the project. */
    bool buildProject();

    /** @brief Handle window events. */
    void pollEvents();

    /** @brief Handle the keyboard shortcuts of the menu. */
    void handleShortcuts();

    /** @brief Draw the error popup while there is an error to show. */
    void drawErrorPopup();

    /** @brief Draw the info popup while there is an info to show. */
    void drawInfoPopup();

    /**
     * @brief Run action now if dirty is false, else keep it until the unsaved
     * changes modal is answered.
     */
    void confirmUnsaved(bool dirty, std::function<void()> action);

    /** @brief Draw the unsaved changes modal while an action waits for it. */
    void drawUnsavedModal();

    /** @brief Draw the new scene modal while there is one to show. */
    void drawNewSceneModal();

    /** @brief Draw the new project modal while there is one to show. */
    void drawNewProjectModal();

    /** @brief Draw and create the dockspace. */
    static void drawDockspace();

    /** @brief Draw the menu bar. */
    void drawMenuBar();

    /** @brief Draw the File menu. */
    void drawFileMenu();

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

    /**
     * @brief Whether the entity has children once its prefab is resolved.
     * chain holds the prefabs being resolved, as in drawPrefabChildren.
     */
    bool hasChildren(const nlohmann::ordered_json& entity,
                     std::vector<std::string>& chain);

    /**
     * @brief Label of an entity: its name, its prefab's name, or "Entity N"
     * with N its index among its siblings.
     */
    std::string entityLabel(const nlohmann::ordered_json& entity,
                            std::size_t index);

    /*
     * JSON widgets (Widgets.cpp). Each draws the widget fitting the shape of
     * value, and returns true if the user changed it.
     */

    /**
     * @brief Draw the widget of any JSON value, recursively.
     *
     * field is the key that tells what the value is (see TypeTable::getField
     * and kAssetKeys): its own key, the key of its array for an item, or the
     * key of its map for an entry. Empty means label.
     */
    static bool editValue(const std::string& label,
                          nlohmann::ordered_json& value,
                          std::string_view field = {});

    /** @brief Each key of an object, without a tree node around them. */
    static bool editFields(nlohmann::ordered_json& object);

    /** @brief Object: map, vector, rect, colour, or a tree of its keys. */
    static bool editObject(const std::string& label,
                           nlohmann::ordered_json& value,
                           std::string_view field);

    /**
     * @brief Object of free keys (TypeTable entryDefault): one tree node per
     * entry, renamable and removable, and an add button.
     */
    static bool editMap(const std::string& label, nlohmann::ordered_json& map,
                        std::string_view field);

    /** @brief The entry of a map: its name, then its value. */
    static bool editMapEntry(const std::string& key,
                             nlohmann::ordered_json& value,
                             std::string_view field,
                             std::optional<std::string>& renamed);

    /** @brief kNewEntry, numbered to be a key map doesn't have yet. */
    static std::string newEntryKey(const nlohmann::ordered_json& map);

    /** @brief Copy of object with the key from renamed to newKey, same order.
     */
    static nlohmann::ordered_json renameKey(
        const nlohmann::ordered_json& object, const std::string& from,
        const std::string& newKey);

    /**
     * @brief Object of numbers named keys, as one drag widget. Rects of
     * integers stay integers, vectors are dragged as floats.
     */
    static bool editNumbers(const std::string& label,
                            nlohmann::ordered_json& value,
                            std::span<const std::string_view> keys);

    /** @brief {r, g, b, a} object of 0-255 integers, as a colour picker. */
    static bool editColor(const std::string& label,
                          nlohmann::ordered_json& value);

    /** @brief Number: integers stay integers, floats are dragged. */
    static bool editNumber(const std::string& label,
                           nlohmann::ordered_json& value);

    /** @brief String, accepting a dropped file path for an asset field. */
    static bool editString(const std::string& label,
                           nlohmann::ordered_json& value,
                           std::string_view field);

    /** @brief String or null picked among choices, null as kNoChoice. */
    static bool editChoice(const std::string& label,
                           nlohmann::ordered_json& value,
                           const std::vector<std::string>& choices);

    /** @brief Array in a tree node, see editItems. */
    static bool editArray(const std::string& label,
                          nlohmann::ordered_json& value,
                          std::string_view field);

    /**
     * @brief One row per item, with remove buttons, and an add button for a
     * copy of the last item (or the itemDefault of field if empty).
     */
    static bool editItems(nlohmann::ordered_json& array,
                          std::string_view field);

    /**
     * @brief Text field editing text through a buffer of kTextSize.
     * @return true, with text updated, if the user changed it.
     */
    static bool inputText(const std::string& label, std::string& text);

    /** @brief Whether key names a string holding a project file path. */
    static bool isAssetKey(std::string_view key);

    /** @brief Whether value is an object of exactly keys, all numbers. */
    static bool hasShape(const nlohmann::ordered_json& value,
                         std::span<const std::string_view> keys);

    /**
     * @brief Stores value in number: as an integer if number was one and
     * value has no fraction, else as the shortest double of the float.
     */
    static void setFloat(nlohmann::ordered_json& number, float value);

    /** @brief Draw the viewport panel. */
    static void drawViewport();

    /** @brief Draw the inspector panel of the selection. */
    void drawInspector();

    /** @brief Draw the inspector of the World, scene resources or entity. */
    static void drawSelectionInspector(Project& project,
                                       const Selection& selection);

    /**
     * @brief Edit the startScene and scenes (name to file path) of the
     * configuration. Renaming the start scene renames startScene too.
     */
    static bool editConfigScenes(nlohmann::ordered_json& config);

    /**
     * @brief Combo of the scene names for startScene, with an error if it is
     * not one of them (the engine refuses to load it).
     */
    static bool editStartScene(nlohmann::ordered_json& config);

    /**
     * @brief If the scene named startScene was renamed from before to after,
     * set startScene to its new name.
     */
    static void followStartScene(nlohmann::ordered_json& config,
                                 const nlohmann::ordered_json& before,
                                 const nlohmann::ordered_json& after);

    /**
     * @brief Draw the inspector of a scene entity: its name, persistent flag
     * (root entities only) and components. Marks document dirty on change.
     */
    static void drawEntityInspector(Project& project, Document& document,
                                    const Pointer& pointer);

    /**
     * @brief Edit the components of a prefab instance as resolved, then store
     * only what differs from its prefab as its own components.
     */
    static bool editPrefabComponents(Project& project,
                                     nlohmann::ordered_json& entity);

    /**
     * @brief Edit the object owner[key] with editTypedValues. owner[key] is
     * only written, as an object, if something changed.
     */
    static bool editTypedMap(nlohmann::ordered_json& owner,
                             const std::string& key, const TypedMapEdit& edit);

    /**
     * @brief Edit an object of typed values (resources or components): one
     * removable collapsing header per value, and an add popup of types.
     */
    static bool editTypedValues(nlohmann::ordered_json& map,
                                const TypedMapEdit& edit);

    /**
     * @brief Edit the fields of the typed value name. On a prefab instance,
     * its overridden fields are marked and get a revert button.
     */
    static bool editTypedValue(const std::string& name,
                               nlohmann::ordered_json& value,
                               const TypedMapEdit& edit);

    /**
     * @brief value with the fields of the defaults of its type name it lacks
     * (optional fields like InputManager.axes), so they can be edited. They
     * are only written to the file once something is edited.
     */
    static nlohmann::ordered_json withDefaults(
        const nlohmann::ordered_json& value, const std::string& name,
        const std::vector<TypeInfo>& types);

    /**
     * @brief Draw the add button and popup of editTypedValues, listing the
     * types not in map yet.
     * @return The type picked, if any.
     */
    static std::optional<TypeInfo> drawAddTypePopup(
        const nlohmann::ordered_json& map, const TypedMapEdit& edit);

    std::optional<Project> _project;
    std::optional<Selection> _selection;
    bool _newSceneModal{false};
    std::string _newProjectPath{};
    std::optional<HierarchyAction> _hierarchyAction;
    Pointer _renaming;
    std::array<char, kTextSize> _textBuffer{};  // Rename and new modals
    std::string _iniPath;
    std::string _error;
    std::string _info;
    std::function<void()> _pendingAction;  // Waits for the unsaved modal
    sf::RenderWindow _window;
    sf::Clock _clock;
    bool _running = true;
};

}  // namespace ampersand::editor
