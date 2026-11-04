#pragma once

#include <array>
#include <cstddef>
#include <cstdint>
#include <map>
#include <string>

namespace ampersand::core {

/**
 * @brief Metrics of the game loop, filled every frame by the Scheduler if
 * the game added this resource (nothing is measured otherwise).
 *
 * Times are measured with the real clock, not with the dt given to the
 * Scheduler: a server with a fixed dt still sees how long its ticks take.
 */
struct Stats {
    /** @brief Number of frame times kept in history. */
    static constexpr std::size_t kHistory = 120;

    /** @brief Milliseconds between the start of the previous frame and the
     * start of this one (0 on the first frame). */
    float frameMs{};
    /** @brief Milliseconds spent in the last update: systems, CommandQueue
     * and event queues. */
    float updateMs{};
    /** @brief Frames per second, averaged over about one second (0 during
     * the first second). */
    float fps{};
    /** @brief Entities alive at the end of the last update. */
    std::uint32_t entities{};
    /** @brief The last frameMs values, from the oldest to the newest (the
     * last one). */
    std::array<float, kHistory> history{};
    /** @brief Metrics added by the modules or the game, by name
     * ("game.ping"). Never cleared by the Scheduler. */
    std::map<std::string, float> values;
};

}  // namespace ampersand::core
