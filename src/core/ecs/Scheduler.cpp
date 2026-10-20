#include <ampersand/core/ecs/Scheduler.hpp>
#include <ampersand/core/ecs/World.hpp>
#include <ampersand/core/resources/Clock.hpp>
#include <cmath>
#include <stdexcept>

namespace ampersand::core::ecs {

Scheduler::Scheduler(World& world) : _world(world) {
    if (!_world.hasResource<Clock>()) {
        _world.addResource<Clock>();
    }
}

void Scheduler::update(float deltaTime) {
    if (!std::isfinite(deltaTime) || deltaTime < 0.0F) {
        throw std::invalid_argument(
            "Scheduler::update: deltaTime must be finite and not negative");
    }
    auto& clock = _world.getResource<Clock>();
    clock.dt = deltaTime;
    clock.elapsed += deltaTime;
    for (const auto& system : _systems) {
        system->update(_world);
    }
}

}  // namespace ampersand::core::ecs
