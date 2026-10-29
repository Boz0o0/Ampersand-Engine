#pragma once

#include <ampersand/core/ecs/Entity.hpp>
#include <ampersand/core/math/Vec3.hpp>

namespace ampersand::core {

namespace ecs {
class World;
}  // namespace ecs

/**
 * @brief Position, scale, rotation, and origin for a 3D object, relative to
 * the parent entity (see World::setParent).
 *
 * For a root entity, the local transform is also the world one. Use
 * worldTransform to place a child in the world.
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

/**
 * @brief Computes the transform of an entity in the world, by composing its
 * Transform with the ones of all its parents.
 *
 * @param world World owning the entity.
 * @param entity Entity to place.
 * @return The entity transform in world space.
 * @throws InvalidEntity if the entity is dead, null or out of range.
 * @throws ComponentNotFound if the entity has no Transform.
 */
[[nodiscard]] Transform worldTransform(ecs::World& world, ecs::Entity entity);

}  // namespace ampersand::core
