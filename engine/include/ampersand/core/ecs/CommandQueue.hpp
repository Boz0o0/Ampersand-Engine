#pragma once

#include <ampersand/core/ecs/Entity.hpp>
#include <functional>
#include <vector>

namespace ampersand::core::ecs {

class World;

/**
 * @brief Changes to the World recorded during a frame, and applied later, in
 * the order they were recorded.
 *
 * Used as a resource of the World, so a system can destroy an entity while it
 * loops over owners<T>(). The Scheduler applies it at the end of each frame.
 * Without a Scheduler, the game calls apply itself.
 */
class CommandQueue {
public:
    /**
     * @brief Destroys an entity when the queue is applied.
     *
     * Until then, the entity stays alive. An entity already dead when the
     * queue is applied (destroyed twice, for instance) is skipped.
     *
     * @param entity Entity to destroy.
     */
    void destroy(Entity entity);

    /**
     * @brief Runs every command in the order it was recorded, then empties the
     * queue.
     *
     * @param world World the commands change.
     */
    void apply(World& world);

private:
    std::vector<std::function<void(World&)>> _commands;
};

}  // namespace ampersand::core::ecs
