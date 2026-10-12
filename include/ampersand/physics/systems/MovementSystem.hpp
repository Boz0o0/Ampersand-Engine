#pragma once

#include <ampersand/core/ecs/ISystem.hpp>

namespace ampersand::physics {

/**
 * @brief Moves every entity that has a Velocity and a Transform, using the
 * dt of the Clock resource.
 */
class MovementSystem final : public core::ecs::ISystem {
public:
    /**
     * @brief Adds velocity * dt to the x and y of each Transform position.
     *
     * @param world World holding the entities and the Clock resource.
     * @throws ResourceNotFound if the World has no Clock.
     */
    void update(core::ecs::World& world) override;
};

}  // namespace ampersand::physics
