#ifdef AMPERSAND_MODULE_SCRIPTING

#include <ampersand/scene/resources/SceneManager.hpp>
#include <ampersand/scripting/components/Scripts.hpp>
#include <filesystem>
#include <string>
#include <utility>

namespace ampersand::scene {

void SceneManager::loadScripts(core::ecs::World& world,
                               core::ecs::Entity entity,
                               const json& data) const {
    scripting::Scripts scripts;
    for (const auto& path : data.at("scripts")) {
        scripts.scripts.push_back(
            (std::filesystem::path(_gameRoot) / path.get<std::string>())
                .string());
    }
    world.add(entity, std::move(scripts));
}

void SceneManager::registerScripting() {
    registerComponent(
        "Scripts",
        [this](core::ecs::World& world, core::ecs::Entity entity,
               const json& data) { loadScripts(world, entity, data); });
}

void SceneManager::bindScripting(scripting::ScriptingSystem& scripting,
                                 core::ecs::World& world) {
    scripting.lua().set_function("instantiate",
                                 [this, &world](const std::string& path) {
                                     return instantiate(world, path);
                                 });
    scripting.lua().set_function(
        "loadScene", [this, &scripting](std::string name) {
            scripting.defer(
                [this, name = std::move(name)](core::ecs::World& world) {
                    loadScene(world, name);
                });
        });
    scripting.lua().set_function("getCurrentScene",
                                 [this] { return getCurrentScene(); });
}

}  // namespace ampersand::scene

#endif
