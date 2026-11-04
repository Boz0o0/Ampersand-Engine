#include <ampersand/core/components/Transform.hpp>
#include <ampersand/core/ecs/World.hpp>
#include <ampersand/core/hierarchy/WorldTransform.hpp>

namespace ampersand::core {

Transform worldTransform(ecs::World& world, ecs::Entity entity) {
    Transform result = world.get<Transform>(entity);
    for (ecs::Entity parent = world.parent(entity); parent != ecs::Entity{};
         parent = world.parent(parent)) {
        if (world.has<Transform>(parent)) {
            result = compose(world.get<Transform>(parent), result);
        }
    }
    return result;
}

}  // namespace ampersand::core
