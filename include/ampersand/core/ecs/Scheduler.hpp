#pragma once

#include <ampersand/core/ecs/EventQueue.hpp>
#include <ampersand/core/ecs/ISystem.hpp>
#include <ampersand/core/ecs/World.hpp>
#include <functional>
#include <memory>
#include <utility>
#include <vector>

namespace ampersand::core::ecs {

/**
 * @brief Runs the systems of a World in the order they were added, one frame
 * at a time, keeps the Clock resource up to date and empties the event queues
 * at the end of each frame.
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
     * @brief Adds an EventQueue<T> resource to the World, emptied at the end of
     * every frame.
     *
     * An event is read by the systems that run after its sender in the same
     * frame. An event sent between two frames (outside update) is read during
     * the next one.
     *
     * @tparam T Event type.
     * @return The queue, owned by the World.
     * @throws ResourceAlreadyExists if the World already has an EventQueue<T>.
     */
    template <typename T>
    EventQueue<T>& addEventQueue() {
        auto& queue = _world.addResource<EventQueue<T>>();
        _endOfFrame.emplace_back(
            [](World& world) { world.getResource<EventQueue<T>>().clear(); });
        return queue;
    }

    /**
     * @brief Runs one frame: sets the Clock (dt = deltaTime, elapsed +=
     * deltaTime), runs every system in order, then empties the event queues.
     *
     * An exception thrown by a system stops the frame and goes to the caller:
     * the event queues are not emptied.
     *
     * @param deltaTime Seconds since the previous frame.
     * @throws std::invalid_argument if deltaTime is negative or not finite.
     */
    void update(float deltaTime);

private:
    World& _world;
    std::vector<std::unique_ptr<ISystem>> _systems;
    /** @brief Work done after the systems, at the end of each frame. */
    std::vector<std::function<void(World&)>> _endOfFrame;
};

}  // namespace ampersand::core::ecs
