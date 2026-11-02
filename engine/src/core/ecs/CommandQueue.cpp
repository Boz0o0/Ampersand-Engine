#include <ampersand/core/ecs/CommandQueue.hpp>
#include <ampersand/core/ecs/World.hpp>
#include <utility>

namespace ampersand::core::ecs {

void CommandQueue::destroy(Entity entity) {
    _commands.emplace_back([entity](World& world) {
        if (world.alive(entity)) {
            world.destroy(entity);
        }
    });
}

void CommandQueue::apply(World& world) {
    auto commands = std::exchange(_commands, {});
    for (const auto& command : commands) {
        command(world);
    }
}

}  // namespace ampersand::core::ecs
