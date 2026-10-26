#include <algorithm>
#include <ampersand/scene/resources/SceneManager.hpp>
#include <filesystem>
#include <fstream>
#include <ranges>
#include <stdexcept>

namespace ampersand::scene {

SceneManager::SceneManager() { registerBuiltins(); }

void SceneManager::registerBuiltins() {
    registerComponent<core::Transform>("Transform");
#ifdef AMPERSAND_MODULE_RENDER
    registerRender();
#endif
#ifdef AMPERSAND_MODULE_PHYSICS
    registerPhysics();
#endif
#ifdef AMPERSAND_MODULE_AUDIO
    registerAudio();
#endif
#ifdef AMPERSAND_MODULE_INPUT
    registerInput();
#endif
}

void SceneManager::registerResource(const std::string& name,
                                    ResourceLoader load,
                                    ResourceRemover remove) {
    _resources[name] = {.load = std::move(load), .remove = std::move(remove)};
}

void SceneManager::registerComponent(const std::string& name,
                                     ComponentLoader load) {
    _components[name] = std::move(load);
}

std::uint32_t SceneManager::asset(const std::string& kind,
                                  const std::string& path,
                                  const AssetLoader& load) {
    auto& cache = _assets[kind];
    const auto found = cache.find(path);
    if (found != cache.end()) {
        return found->second;
    }
    const auto loaded =
        load((std::filesystem::path(_configRoot) / path).string());
    if (loaded == 0) {
        throw std::runtime_error("Cannot load " + kind + " '" + path + "'");
    }
    cache[path] = loaded;
    return loaded;
}

void SceneManager::forgetAssets(const std::string& kind) {
    _assets.erase(kind);
}

void SceneManager::dontDestroyOnLoad(core::ecs::World& world,
                                     core::ecs::Entity entity) {
    if (world.parent(entity) != core::ecs::Entity{}) {
        throw std::invalid_argument(
            "Entity must be a root (no parent) to be marked as persistent.");
    }
    if (std::ranges::find(_persistentRoots, entity) == _persistentRoots.end()) {
        _persistentRoots.push_back(entity);
    }
}

void SceneManager::loadConfig(core::ecs::World& world,
                              const std::string& path) {
    try {
        const json config = readJson(path);
        unloadConfig(world);
        _configRoot = config.value("configRoot", std::string{});
        _scenes = config.value("scenes", std::map<std::string, std::string>{});
        loadResources(world, config.value("resources", json{}),
                      _configResources);
        loadScene(world, config.at("startScene").get<std::string>());
    } catch (const std::exception& e) {
        throw std::runtime_error("Failed to load configuration '" + path +
                                 "': " + e.what());
    }
}

void SceneManager::loadScene(core::ecs::World& world,
                             const std::string& sceneName) {
    try {
        const auto path = _scenes.find(sceneName);
        if (path == _scenes.end()) {
            throw std::runtime_error("Unknown scene");
        }
        const json scene = readFile(path->second);
        unloadScene(world);
        _currentScene = sceneName;
        loadResources(world, scene.value("resources", json{}), _sceneResources);
        loadEntities(world, scene.value("entities", json{}));
    } catch (const std::exception& e) {
        throw std::runtime_error("Failed to load scene '" + sceneName +
                                 "': " + e.what());
    }
}

core::ecs::Entity SceneManager::instantiate(core::ecs::World& world,
                                            const std::string& path) {
    try {
        const auto entity =
            createEntity(world, resolvePrefab({{"prefab", path}}), {});
        _sceneEntities.push_back(entity);
        return entity;
    } catch (const std::exception& e) {
        throw std::runtime_error("Failed to instantiate '" + path +
                                 "': " + e.what());
    }
}

void SceneManager::setScene(const std::string& name, const std::string& path) {
    _scenes[name] = path;
}

json SceneManager::readJson(const std::string& path) {
    std::ifstream file(path);
    if (!file) {
        throw std::runtime_error("Cannot open '" + path + "'");
    }
    json data = json::parse(file);
    const auto version = data.at("version").get<std::int32_t>();
    if (version != kVersion) {
        throw std::runtime_error("'" + path + "' has version " +
                                 std::to_string(version) + ", expected " +
                                 std::to_string(kVersion));
    }
    return data;
}

json SceneManager::readFile(const std::string& path) const {
    return readJson((std::filesystem::path(_configRoot) / path).string());
}

json SceneManager::resolvePrefab(const json& description,
                                 std::vector<std::string> chain) const {
    json result = description;
    if (description.contains("prefab")) {
        const auto path =
            std::filesystem::path(description.at("prefab").get<std::string>())
                .lexically_normal()
                .string();
        if (std::ranges::find(chain, path) != chain.end()) {
            throw std::runtime_error("Prefab '" + path + "' uses itself");
        }
        chain.push_back(path);
        result = resolvePrefab(readFile(path), chain);
        chain.pop_back();
        result["components"].merge_patch(
            description.value("components", json::object()));
    } else {
        result["children"] = json::array();
    }
    for (const auto& child : description.value("children", json::array())) {
        result["children"].push_back(resolvePrefab(child, chain));
    }
    return result;
}

core::ecs::Entity SceneManager::createEntity(core::ecs::World& world,
                                             const json& resolved,
                                             core::ecs::Entity parent) {
    const auto entity = world.create();
    try {
        if (parent != core::ecs::Entity{}) {
            world.setParent(entity, parent);
        }
        loadComponents(world, entity,
                       resolved.value("components", json::object()));
        for (const auto& child : resolved.at("children")) {
            createEntity(world, child, entity);
        }
    } catch (...) {
        world.destroy(entity);
        throw;
    }
    return entity;
}

void SceneManager::loadComponents(core::ecs::World& world,
                                  core::ecs::Entity entity,
                                  const json& components) {
    for (const auto& [name, data] : components.items()) {
        const auto loader = _components.find(name);
        if (loader == _components.end()) {
            throw std::runtime_error("No loader registered for component '" +
                                     name + "'");
        }
        try {
            loader->second(world, entity, data);
        } catch (const std::exception& e) {
            throw std::runtime_error("Failed to load component '" + name +
                                     "': " + e.what());
        }
    }
}

void SceneManager::loadResources(core::ecs::World& world, const json& resources,
                                 std::vector<std::string>& loaded) {
    if (resources.is_null()) {
        return;
    }
    if (!resources.is_object()) {
        throw std::runtime_error("'resources' must be an object or null");
    }
    for (const auto& [name, data] : resources.items()) {
        const auto entry = _resources.find(name);
        if (entry == _resources.end()) {
            throw std::runtime_error("No loader registered for resource '" +
                                     name + "'");
        }
        try {
            entry->second.load(world, data);
            loaded.push_back(name);
        } catch (const std::exception& e) {
            throw std::runtime_error("Failed to load resource '" + name +
                                     "': " + e.what());
        }
    }
}

void SceneManager::unloadResources(core::ecs::World& world,
                                   std::vector<std::string>& loaded) {
    for (const auto& name : std::views::reverse(loaded)) {
        _resources.at(name).remove(world);
    }
    loaded.clear();
}

void SceneManager::loadEntities(core::ecs::World& world, const json& entities) {
    if (entities.is_null()) {
        return;
    }
    if (!entities.is_array()) {
        throw std::runtime_error("'entities' must be an array or null");
    }
    std::size_t index = 0;
    for (const auto& description : entities) {
        const bool persistent = description.value("persistent", false);
        const auto key = _currentScene + "#" + std::to_string(index++);
        const auto created = _persistentEntries.find(key);
        if (persistent && created != _persistentEntries.end() &&
            world.alive(created->second)) {
            continue;
        }
        const auto entity = createEntity(world, resolvePrefab(description), {});
        _sceneEntities.push_back(entity);
        if (persistent) {
            dontDestroyOnLoad(world, entity);
            _persistentEntries[key] = entity;
        }
    }
}

bool SceneManager::isPersistent(const core::ecs::World& world,
                                core::ecs::Entity entity) const {
    while (world.parent(entity) != core::ecs::Entity{}) {
        entity = world.parent(entity);
    }
    return std::ranges::find(_persistentRoots, entity) !=
           _persistentRoots.end();
}

void SceneManager::unloadScene(core::ecs::World& world) {
    std::erase_if(_persistentRoots, [&world](core::ecs::Entity entity) {
        return !world.alive(entity);
    });
    for (const auto& entity : _sceneEntities) {
        if (world.alive(entity) && !isPersistent(world, entity)) {
            world.destroy(entity);
        }
    }
    _sceneEntities.clear();
    unloadResources(world, _sceneResources);
}

void SceneManager::unloadConfig(core::ecs::World& world) {
    for (const auto& entity : _persistentRoots) {
        if (world.alive(entity)) {
            world.destroy(entity);
        }
    }
    _persistentRoots.clear();
    _persistentEntries.clear();
    unloadScene(world);
    unloadResources(world, _configResources);
    _scenes.clear();
    _assets.clear();
    _currentScene.clear();
}

}  // namespace ampersand::scene
