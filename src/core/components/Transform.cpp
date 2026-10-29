#include <ampersand/core/components/Transform.hpp>
#include <cmath>
#include <numbers>

namespace ampersand::core {

Transform compose(const Transform& parent, const Transform& child) {
    const float radians =
        parent.rotationDegrees.z * std::numbers::pi_v<float> / kHalfTurnDegrees;
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

}  // namespace ampersand::core
