#pragma once

#include <ampersand/core/math/Vec3.hpp>

namespace ampersand::core {

/**
 * @brief Position, scale, rotation, and origin for a 3D object, relative to
 * the parent entity (see World::setParent).
 *
 * For a root entity, the local transform is also the world one. Use
 * worldTransform (core/hierarchy/WorldTransform.hpp) to place a child in the
 * world.
 */
struct Transform {
    math::Vec3f position;
    math::Vec3f scale{1.0F, 1.0F, 1.0F};
    math::Vec3f rotationDegrees;
    math::Vec3f origin;
};

/**
 * @brief Moves a child transform out of its parent: the result is in the
 * space the parent transform is expressed in (the world for a root).
 *
 * @param parent Transform of the parent.
 * @param child Transform of the child, relative to the parent.
 * @return The child transform, in the space of the parent transform.
 */
[[nodiscard]] Transform compose(const Transform& parent,
                                const Transform& child);

}  // namespace ampersand::core
