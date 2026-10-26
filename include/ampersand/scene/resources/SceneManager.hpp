#pragma once

#include <ampersand/core/ecs/World.hpp>
#include <ampersand/scene/serialization/Serializers.hpp>
#include <cstdint>
#include <functional>
#include <map>
#include <nlohmann/json.hpp>
#include <string>
#include <vector>

#ifdef AMPERSAND_MODULE_AUDIO
#include <ampersand/audio/IAudioEngine.hpp>
#endif

namespace ampersand::scene {

using nlohmann::json;

/**
 * @brief Where a key of a "resources" block may be given.
 */
enum class ResourceScope : std::uint8_t {
    Any,     // In the configuration or in a scene
    Config,  // Only in the configuration: a scene giving it fails
};

/**
 * @brief Manager for scenes and prefabs. Loads a configuration file that
 * contains the paths to the scenes and prefabs. Can load a scene, instantiate a
 * prefab, and set a scene in the SceneManager scenes map.
 *
 * The engine types (resources such as Camera or Renderer, components such as
 * Transform or Sprite) are registered on construction, for the enabled
 * modules (src/scene/loaders). The game only registers its own types with
 * registerResource and registerComponent. Renderer, AudioListener and
 * InputManager are only allowed in the configuration (ResourceScope::Config):
 * the systems using them need them in every scene.
 *
 * Configuration format: {"version", "configRoot", "startScene", "scenes":
 * {name: path}, "resources": {...}}. Scene format: {"version", "resources":
 * {...}, "entities": [...]}. Only "version" and "startScene" are mandatory.
 *
 * Entity format: {"prefab": path, "persistent": bool, "components": {...},
 * "children": [...]}, every key optional ("name" is ignored). Every field of
 * a component is mandatory. The components of the scene are merged field by
 * field over the ones of the prefab (a null component removes it), so a scene
 * only gives the fields it overrides. Paths are relative to configRoot. A
 * persistent entry is created once: loading its scene again does not create
 * it twice while it is alive.
 */
class SceneManager {
public:
    static constexpr std::int32_t kVersion = 1;  // Version of the SceneManager

    /**
     * @brief Creates a SceneManager with the engine types already registered.
     */
    SceneManager();

    // The built-in loaders keep a reference to this SceneManager.
    SceneManager(const SceneManager&) = delete;
    SceneManager& operator=(const SceneManager&) = delete;
    SceneManager(SceneManager&&) = delete;
    SceneManager& operator=(SceneManager&&) = delete;
    ~SceneManager() = default;

    /**
     * @brief Builds a resource in the World from its JSON configuration.
     */
    using ResourceLoader =
        std::function<void(core::ecs::World& world, const json& data)>;

    /**
     * @brief Removes a resource from the World (does nothing if it is
     * missing).
     */
    using ResourceRemover = std::function<void(core::ecs::World& world)>;

    /**
     * @brief How a key of a "resources" block is loaded and removed.
     */
    struct ResourceEntry {
        ResourceLoader load;
        ResourceRemover remove;
        ResourceScope scope{ResourceScope::Any};
    };

    /**
     * @brief Registers the functions called for a key of a "resources" block
     * (overrides an existing entry, built-in ones included).
     *
     * Only needed for the types of the game: the engine types are registered
     * on construction. Entries are kept across loadConfig calls.
     *
     * @param name Key of the resource in the "resources" block.
     * @param load Function building the resource from its JSON value.
     * @param remove Function removing it when its scene is unloaded.
     * @param scope Config if a scene may not give it.
     */
    void registerResource(const std::string& name, ResourceLoader load,
                          ResourceRemover remove,
                          ResourceScope scope = ResourceScope::Any);

    /**
     * @brief Registers resource type T with a custom loader. It is removed
     * with world.removeResource<T>().
     *
     * @tparam T Resource type.
     * @param name Key of the resource in the "resources" block.
     * @param load Function building the resource from its JSON value.
     * @param scope Config if a scene may not give it.
     */
    template <typename T>
    void registerResource(const std::string& name, ResourceLoader load,
                          ResourceScope scope = ResourceScope::Any) {
        registerResource(
            name, std::move(load),
            [](core::ecs::World& world) {
                if (world.hasResource<T>()) {
                    world.removeResource<T>();
                }
            },
            scope);
    }

    /**
     * @brief Registers resource type T, built with
     * world.addResource<T>(data.get<T>()).
     *
     * @tparam T Resource type.
     * @param name Key of the resource in the "resources" block.
     * @param scope Config if a scene may not give it.
     */
    template <typename T>
    void registerResource(const std::string& name,
                          ResourceScope scope = ResourceScope::Any) {
        registerResource<T>(
            name,
            [](core::ecs::World& world, const json& data) {
                world.addResource<T>(data.get<T>());
            },
            scope);
    }

    /**
     * @brief Registers resource type T, built from an object that does not
     * come from the JSON and a parsed Data, with
     * world.addResource<T>(dep, data.get<Data>()).
     *
     * @tparam T Resource type.
     * @tparam Data Type parsed from the JSON.
     * @param name Key of the resource in the "resources" block.
     * @param dep Object given to the constructor of T. Must outlive the
     * SceneManager and the World.
     * @param scope Config if a scene may not give it.
     */
    template <typename T, typename Data, typename Dep>
    void registerResource(const std::string& name, Dep& dep,
                          ResourceScope scope = ResourceScope::Any) {
        registerResource<T>(
            name,
            [&dep](core::ecs::World& world, const json& data) {
                world.addResource<T>(dep, data.get<Data>());
            },
            scope);
    }

    /**
     * @brief Adds a component to an entity from its JSON value.
     */
    using ComponentLoader = std::function<void(
        core::ecs::World& world, core::ecs::Entity entity, const json& data)>;

    /**
     * @brief Registers the loader called for a key of a "components" block
     * (overrides an existing loader, built-in ones included).
     *
     * Only needed for the components of the game: the engine components are
     * registered on construction.
     *
     * @param name Key of the component in the "components" block.
     * @param load Function adding the component to the entity.
     */
    void registerComponent(const std::string& name, ComponentLoader load);

    /**
     * @brief Registers component type T (declared with
     * NLOHMANN_DEFINE_TYPE_NON_INTRUSIVE). Every field is mandatory: a missing
     * one throws.
     *
     * @tparam T Component type.
     * @param name Key of the component in the "components" block.
     */
    template <typename T>
    void registerComponent(const std::string& name) {
        registerComponent(
            name, [](core::ecs::World& world, core::ecs::Entity entity,
                     const json& data) { world.add(entity, data.get<T>()); });
    }

    /**
     * @brief Loads an asset from its full path, returns its id (0 on failure).
     */
    using AssetLoader = std::function<std::uint32_t(const std::string& path)>;

    /**
     * @brief Returns the id of an asset, loading it on first use.
     *
     * @param kind Kind of the asset ("texture", "sound"...), one cache each.
     * @param path Path of the asset, relative to configRoot.
     * @param load Loads the asset from its full path.
     * @throws std::runtime_error if the asset cannot be loaded.
     */
    std::uint32_t asset(const std::string& kind, const std::string& path,
                        const AssetLoader& load);

    /**
     * @brief Forgets the ids of a kind of asset, for when the backend that
     * loaded them is removed.
     */
    void forgetAssets(const std::string& kind);

    /**
     * @brief Marks an entity as persistent across scene loads.
     *
     * The entity must be a root (no parent). It is not destroyed when a new
     * scene is loaded, only when a new configuration is loaded, even if the
     * game created it. Its children, and the entities attached under it
     * later, go with it.
     *
     * @param world World owning the entity.
     * @param entity Entity to mark as persistent.
     * @throws std::invalid_argument if the entity has a parent.
     */
    void dontDestroyOnLoad(core::ecs::World& world, core::ecs::Entity entity);

    /**
     * @brief Unloads everything the previous configuration loaded (entities,
     * persistent ones included, and resources), then loads the new
     * configuration file and its start scene.
     *
     * Calls the registered loader of each key of the "resources" block.
     *
     * The file, its "startScene" (a string, listed in "scenes") and the file
     * of the start scene (read from the new configRoot, with no resource only
     * allowed in the configuration) are checked before anything is unloaded:
     * if they are wrong, the previous configuration stays. An error after
     * that unloads everything, so nothing of the previous or the new
     * configuration stays.
     *
     * @throws std::runtime_error if the file is invalid, or if a resource has
     * no registered loader.
     */
    void loadConfig(core::ecs::World& world, const std::string& path);

    /**
     * @brief Unloads the current scene (its entities and resources, in
     * reverse order), then loads a scene and sets it as the current scene.
     *
     * An unknown scene, a file that cannot be read, or a resource only
     * allowed in the configuration leaves the current scene as it was. An error
     * after that removes what the new scene loaded, and leaves no current
     * scene.
     *
     * @throws std::runtime_error if the scene is unknown or invalid.
     */
    void loadScene(core::ecs::World& world, const std::string& sceneName);

    /**
     * @brief Creates an entity (and its children) from a prefab file.
     *
     * The entity belongs to the current scene: it is destroyed when another
     * scene is loaded, unless it is marked with dontDestroyOnLoad.
     *
     * @param world World receiving the entity.
     * @param path Prefab file, relative to configRoot.
     * @return Root entity of the prefab.
     * @throws std::runtime_error if the prefab is invalid.
     */
    core::ecs::Entity instantiate(core::ecs::World& world,
                                  const std::string& path);

    /**
     * @brief Get the current scene name.
     */
    [[nodiscard]] const std::string& getCurrentScene() const {
        return _currentScene;
    }

    /**
     * @brief Get the scenes, by name, with their paths.
     */
    [[nodiscard]] const std::map<std::string, std::string>& getScenes() const {
        return _scenes;
    }

    /**
     * @brief Set a scene in the SceneManager scenes map (overrides existing
     * scene).
     */
    void setScene(const std::string& name, const std::string& path);

private:
    /**
     * @brief Registers the built-in resource and component types for the
     * enabled modules.
     */
    void registerBuiltins();

#ifdef AMPERSAND_MODULE_RENDER
    /**
     * @brief Registers ClearColor, Camera, Renderer, Animation,
     * AnimationStateMachine, Sprite and Text (src/scene/loaders).
     */
    void registerRender();

    /**
     * @brief Creates the backend named by "backend" ("sfml"), then the
     * Renderer from the rest of the block. The backend is kept in the Backends
     * resource under "Renderer".
     */
    static void loadRenderer(core::ecs::World& world, const json& data);

    /**
     * @brief Removes the Renderer and its backend, and forgets the textures
     * and fonts it loaded.
     */
    void removeRenderer(core::ecs::World& world);

    /**
     * @brief Sprite: "texture" is a path, loaded once by the Renderer.
     */
    void loadSprite(core::ecs::World& world, core::ecs::Entity entity,
                    const json& data);

    /**
     * @brief Text: "font" is a path, loaded once by the Renderer.
     */
    void loadText(core::ecs::World& world, core::ecs::Entity entity,
                  const json& data);

    /**
     * @brief Checks one condition of a transition: {"param", "op", "value"}
     * for a float, {"param", "value": bool} for a bool.
     *
     * @throws std::runtime_error if the operator is unknown, or given for a
     * bool.
     */
    static bool conditionHolds(const json& condition,
                               const render::AnimationParams& params);

    /**
     * @brief AnimationStateMachine: "initial", "states", "params" and
     * "transitions" ({"from", "to", "when": [conditions]}).
     */
    static void loadAnimationStateMachine(core::ecs::World& world,
                                          core::ecs::Entity entity,
                                          const json& data);
#endif

#ifdef AMPERSAND_MODULE_PHYSICS
    /**
     * @brief Registers GravityForce, Velocity, Gravity and BoxCollider.
     */
    void registerPhysics();
#endif

#ifdef AMPERSAND_MODULE_INPUT
    /**
     * @brief Registers InputManager.
     */
    void registerInput();

    /**
     * @brief InputManager: named "axes" (Axis) and "buttons" (list of
     * bindings), written with the names of the keys and buttons.
     */
    static void loadInputManager(core::ecs::World& world, const json& data);
#endif

#ifdef AMPERSAND_MODULE_AUDIO
    /**
     * @brief Registers AudioListener, SceneMusic and AudioSource.
     */
    void registerAudio();

    /**
     * @brief Creates the engine named by "backend" ("sfml"), then the
     * AudioListener. The engine is kept in the Backends resource under
     * "AudioListener".
     */
    static void loadAudioListener(core::ecs::World& world, const json& data);

    /**
     * @brief Removes the AudioListener and its engine, and forgets the sounds
     * and musics it loaded.
     */
    void removeAudioListener(core::ecs::World& world);

    /**
     * @brief Returns the id of a sound ("sound") or a music ("music"), loaded
     * once by the audio engine.
     */
    std::uint32_t audioAsset(audio::IAudioEngine& engine,
                             const std::string& kind, const std::string& path);

    /**
     * @brief SceneMusic: "music" is a path, opened once by the audio engine.
     */
    void loadSceneMusic(core::ecs::World& world, const json& data);

    /**
     * @brief AudioSource: "sound" or "music" is a path, loaded once by the
     * audio engine.
     */
    void loadAudioSource(core::ecs::World& world, core::ecs::Entity entity,
                         const json& data);
#endif

    /**
     * @brief Reads a JSON file and checks its version.
     */
    static json readJson(const std::string& path);

    /**
     * @brief Reads a JSON file relative to configRoot and checks its version.
     */
    [[nodiscard]] json readFile(const std::string& path) const;

    /**
     * @brief Reads the file of a scene, relative to configRoot, and checks
     * that it gives no resource only allowed in the configuration.
     *
     * @throws std::runtime_error naming the scene if the file is invalid.
     */
    [[nodiscard]] json readScene(const std::string& sceneName,
                                 const std::string& configRoot,
                                 const std::string& path) const;

    /**
     * @brief Unloads the current scene, then loads a scene already read by
     * readScene and sets it as the current scene. An error removes what the
     * new scene loaded, and leaves no current scene.
     *
     * @throws std::runtime_error naming the scene if a part cannot be loaded.
     */
    void switchScene(core::ecs::World& world, const std::string& sceneName,
                     const json& scene);

    /**
     * @brief Returns the entity description, and the ones of its children,
     * with their prefabs (and the prefabs of those prefabs) applied:
     * components merged, children appended.
     *
     * @param chain Prefabs being applied around this description.
     * @throws std::runtime_error if a prefab uses itself, directly or not.
     */
    [[nodiscard]] json resolvePrefab(const json& description,
                                     std::vector<std::string> chain = {}) const;

    /**
     * @brief Creates an entity, its components and its children from a
     * description returned by resolvePrefab.
     */
    core::ecs::Entity createEntity(core::ecs::World& world,
                                   const json& resolved,
                                   core::ecs::Entity parent);

    /**
     * @brief Calls the registered loader of each key of a "components" block.
     */
    void loadComponents(core::ecs::World& world, core::ecs::Entity entity,
                        const json& components);

    /**
     * @brief Calls the registered loader of each key of a "resources" block
     * (null for none), and appends the key to loaded once it is built.
     */
    void loadResources(core::ecs::World& world, const json& resources,
                       std::vector<std::string>& loaded);

    /**
     * @brief Checks that the "resources" block of a scene gives no resource
     * registered with ResourceScope::Config.
     *
     * @throws std::runtime_error naming the first one found.
     */
    void checkSceneResources(const json& resources) const;

    /**
     * @brief Removes the resources, the last loaded first, and empties the
     * list.
     */
    void unloadResources(core::ecs::World& world,
                         std::vector<std::string>& loaded);

    /**
     * @brief Loads the entities from an "entities" block (null for none).
     */
    void loadEntities(core::ecs::World& world, const json& entities);

    /**
     * @brief Tells if the root of an entity (the entity itself if it has no
     * parent) is persistent.
     */
    [[nodiscard]] bool isPersistent(const core::ecs::World& world,
                                    core::ecs::Entity entity) const;

    /**
     * @brief Destroys the entities of the current scene that are not
     * persistent (or attached under a persistent one), and removes its
     * resources.
     */
    void unloadScene(core::ecs::World& world);

    /**
     * @brief Undoes a scene that failed halfway: destroys every entity it
     * created (persistent ones included), removes its resources, and clears
     * the current scene.
     */
    void abortScene(core::ecs::World& world);

    /**
     * @brief Destroys every entity and removes every resource loaded by the
     * configuration, and forgets its scenes and assets.
     */
    void unloadConfig(core::ecs::World& world);

    std::string _configRoot;    // Path to the root of the configuration files
    std::string _currentScene;  // Name of the current scene
    std::map<std::string, std::string>
        _scenes;  // Map of scene names to their paths
    std::vector<core::ecs::Entity>
        _persistentRoots;  // List of persistent root entities
    std::map<std::string, core::ecs::Entity>
        _persistentEntries;  // Persistent entities by "scene#index" of the
                             // entry that created them, not created twice
    std::map<std::string, ResourceEntry>
        _resources;  // Map of resource names to their load/remove functions
    std::map<std::string, ComponentLoader>
        _components;  // Map of component names to their loaders
    std::map<std::string, std::map<std::string, std::uint32_t>>
        _assets;  // Asset ids by kind, then by path
    std::vector<std::string>
        _configResources;  // Resources loaded by the configuration, removed
                           // when a new configuration is loaded
    std::vector<std::string>
        _sceneResources;  // Resources loaded by the current scene, removed
                          // when a new scene is loaded
    std::vector<core::ecs::Entity>
        _sceneEntities;  // Entities loaded by the current scene, destroyed
                         // when a new scene is loaded
};

}  // namespace ampersand::scene
