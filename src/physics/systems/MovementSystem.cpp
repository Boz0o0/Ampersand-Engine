#include <ampersand/core/components/Transform.hpp>
#include <ampersand/core/ecs/World.hpp>
#include <ampersand/core/resources/Clock.hpp>
#include <ampersand/physics/components/Velocity.hpp>
#include <ampersand/physics/systems/MovementSystem.hpp>

namespace ampersand::physics {

void MovementSystem::update(core::ecs::World& world) {
    const float deltaTime = world.getResource<core::Clock>().dt;
    for (const core::ecs::Entity entity : world.owners<Velocity>()) {
        if (!world.has<core::Transform>(entity)) {
            continue;
        }
        auto& position = world.get<core::Transform>(entity).position;
        const auto& velocity = world.get<Velocity>(entity).value;
        position.x += velocity.x * deltaTime;
        position.y += velocity.y * deltaTime;
    }
}

}  // namespace ampersand::physics
