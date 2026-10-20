#pragma once

namespace ampersand::core {

/**
 * @brief Time of the game loop, filled every frame by the Scheduler (or by
 * the game if it runs its systems itself but not advised).
 */
struct Clock {
    /** @brief Seconds since the previous frame. */
    float dt{};
    /** @brief Seconds since the start of the game. */
    float elapsed{};
};

}  // namespace ampersand::core
