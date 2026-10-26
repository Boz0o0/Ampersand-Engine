#ifdef AMPERSAND_MODULE_INPUT

#include <ampersand/scene/resources/SceneManager.hpp>

namespace ampersand::scene {

void SceneManager::loadInputManager(core::ecs::World& world, const json& data) {
    input::InputManager manager;
    const json axes = data.value("axes", json::object());
    const json buttons = data.value("buttons", json::object());
    for (const auto& [name, axis] : axes.items()) {
        manager.setAxis(name, axis.get<input::Axis>());
    }
    for (const auto& [name, bindings] : buttons.items()) {
        manager.setButton(name, bindings.get<std::vector<input::Binding>>());
    }
    world.addResource<input::InputManager>(std::move(manager));
}

void SceneManager::registerInput() {
    registerResource<input::InputManager>("InputManager", loadInputManager);
}

}  // namespace ampersand::scene

#endif
