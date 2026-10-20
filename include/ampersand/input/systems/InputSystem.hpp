#pragma once

#include <ampersand/core/ecs/ISystem.hpp>
#include <ampersand/input/resources/InputManager.hpp>
#include <functional>
#include <optional>

namespace ampersand::input {

/**
 * @brief Fills the InputManager resource each frame with the events given by
 * a poll function.
 *
 * The input module does not know the window: the game gives the poll function
 * at construction (reading the Renderer resource of the World, a replay, a
 * test...) and handles the other events of the window (closing, resizing)
 * inside it.
 */
class InputSystem final : public core::ecs::ISystem {
public:
    /**
     * @brief Gives the next input event, or std::nullopt when none is left.
     * Receives the World given to update.
     */
    using Poll = std::function<std::optional<Event>(core::ecs::World&)>;

    /**
     * @brief Creates the system.
     *
     * @param poll Function giving the input events of the frame, one per call.
     * Must not be empty.
     */
    explicit InputSystem(Poll poll);

    /**
     * @brief Starts a new frame of the InputManager, then calls the poll
     * function with the World until it returns std::nullopt, applying each
     * event to the InputManager.
     *
     * @param world World holding the InputManager resource.
     * @throws ResourceNotFound if the World has no InputManager.
     */
    void update(core::ecs::World& world) override;

private:
    Poll _poll;
};

}  // namespace ampersand::input
