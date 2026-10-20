#include <ampersand/core/ecs/World.hpp>
#include <ampersand/input/resources/InputManager.hpp>
#include <ampersand/input/systems/InputSystem.hpp>
#include <optional>
#include <utility>

namespace ampersand::input {

InputSystem::InputSystem(Poll poll) : _poll(std::move(poll)) {}

void InputSystem::update(core::ecs::World& world) {
    auto& manager = world.getResource<InputManager>();
    manager.beginFrame();
    while (const std::optional<Event> event = _poll(world)) {
        manager.apply(*event);
    }
}

}  // namespace ampersand::input
