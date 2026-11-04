#pragma once

#include <utility>
#include <vector>

namespace ampersand::core::ecs {

/**
 * @brief Events of type T sent during a frame, to be read by the systems that
 * run after the sender.
 *
 * Used as a resource of the World. The Scheduler empties it at the end of each
 * frame. Without a Scheduler, the game needs
 * to clear itself.
 *
 * @tparam T Event type, defined by the game (e.g. struct Died { Entity
 * entity; };).
 */
template <typename T>
class EventQueue {
public:
    /**
     * @brief Adds an event after the others.
     *
     * @param event The event.
     */
    void send(T event) { _events.push_back(std::move(event)); }

    /** @brief Events sent since the last clear, in the order they were sent. */
    [[nodiscard]] const std::vector<T>& read() const { return _events; }

    /** @brief Removes every event. */
    void clear() { _events.clear(); }

private:
    std::vector<T> _events;
};

}  // namespace ampersand::core::ecs
