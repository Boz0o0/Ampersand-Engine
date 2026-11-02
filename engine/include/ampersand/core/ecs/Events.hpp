#pragma once

#include <ampersand/core/ecs/Entity.hpp>

namespace ampersand::core::ecs {

/**
 * @brief An entity just created, with its components and its children.
 */
struct EntityCreated {
    /** @brief The created entity. */
    Entity entity;
};

/**
 * @brief An entity about to be destroyed, sent when its destruction is
 * requested. It stays alive until the CommandQueue is applied.
 */
struct EntityDestroyed {
    /** @brief The entity, still valid during the frame it was sent. */
    Entity entity;
};

}  // namespace ampersand::core::ecs
