#pragma once

#include <ampersand/core/ecs/ISystem.hpp>
#include <memory>
#include <utility>
#include <vector>

namespace ampersand::core::ecs {

class World;

/**
 * @brief Runs the systems of a World in the order they were added, one frame
 * at a time, and keeps the Clock resource up to date.
 *
 * The World must outlive the Scheduler.
 */
class Scheduler {
public:
    /**
     * @brief Creates a Scheduler with no system, and adds a Clock resource to
     * the World if it has none.
     *
     * @param world World given to every system.
     */
    explicit Scheduler(World& world);

    /**
     * @brief Creates a system and adds it after the others.
     *
     * Do not call it from a system while a frame is running.
     *
     * @tparam T System type, derived from ISystem.
     * @param args Arguments given to the constructor of T.
     * @return The system, owned by the Scheduler.
     */
    template <typename T, typename... Args>
    T& add(Args&&... args) {
        auto system = std::make_unique<T>(std::forward<Args>(args)...);
        T& added = *system;
        _systems.push_back(std::move(system));
        return added;
    }

    /**
     * @brief Runs one frame: sets the Clock (dt = deltaTime, elapsed +=
     * deltaTime), then runs every system in order.
     *
     * An exception thrown by a system stops the frame and goes to the caller.
     *
     * @param deltaTime Seconds since the previous frame.
     * @throws std::invalid_argument if deltaTime is negative or not finite.
     */
    void update(float deltaTime);

private:
    World& _world;
    std::vector<std::unique_ptr<ISystem>> _systems;
};

}  // namespace ampersand::core::ecs
