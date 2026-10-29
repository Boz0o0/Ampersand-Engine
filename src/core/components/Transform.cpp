#include <ampersand/core/components/Transform.hpp>
#include <ampersand/core/ecs/World.hpp>
#include <cmath>
#include <numbers>

namespace ampersand::core {

Transform compose(const Transform& parent, const Transform& child) {
    const float radians =
        parent.rotationDegrees.z * std::numbers::pi_v<float> / 180.0F;
    const float cos = std::cos(radians);
    const float sin = std::sin(radians);
    const float offsetX = (child.position.x - parent.origin.x) * parent.scale.x;
    const float offsetY = (child.position.y - parent.origin.y) * parent.scale.y;
    return {
        .position =
            {
                parent.position.x + (offsetX * cos) - (offsetY * sin),
                parent.position.y + (offsetX * sin) + (offsetY * cos),
                parent.position.z + child.position.z,
            },
        .scale =
            {
                parent.scale.x * child.scale.x,
                parent.scale.y * child.scale.y,
                parent.scale.z * child.scale.z,
            },
        .rotationDegrees =
            {
                parent.rotationDegrees.x + child.rotationDegrees.x,
                parent.rotationDegrees.y + child.rotationDegrees.y,
                parent.rotationDegrees.z + child.rotationDegrees.z,
            },
        .origin = child.origin,
    };
}

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
