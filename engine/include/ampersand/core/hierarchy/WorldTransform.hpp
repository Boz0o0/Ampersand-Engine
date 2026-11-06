#pragma once

#include <ampersand/core/components/Transform.hpp>
#include <ampersand/core/ecs/Entity.hpp>

namespace ampersand::core {

namespace ecs {
class World;
}  // namespace ecs

/**
 * @brief Computes the transform of an entity in the world, by composing its
 * Transform with the ones of all its parents (see compose).
 *
 * Parents without a Transform are skipped.
 *
 * @param world World owning the entity.
 * @param entity Entity to place.
 * @return The entity transform in world space.
 * @throws InvalidEntity if the entity is dead, null or out of range.
 * @throws ComponentNotFound if the entity has no Transform.
 */
[[nodiscard]] Transform worldTransform(ecs::World& world, ecs::Entity entity);

}  // namespace ampersand::core
