#pragma once

#include <chrono>
#include <optional>

namespace ampersand::core {

/**
 * @brief Measures the real time between two frames, to give to
 * Scheduler::update.
 */
class FrameTimer {
public:
    /** @brief Default maxDt, in seconds. */
    static constexpr float kDefaultMaxDt = 0.1F;

    /**
     * @brief Creates a timer.
     *
     * @param maxDt Highest value tick returns, in seconds. After a long frame
     * (window moved, slow machine), the game slows down instead of jumping.
     * @throws std::invalid_argument if maxDt is not positive or not finite.
     */
    explicit FrameTimer(float maxDt = kDefaultMaxDt);

    /**
     * @brief Returns the seconds since the previous call, at most maxDt.
     *
     * @return 0 on the first call.
     */
    float tick();

private:
    float _maxDt;
    std::optional<std::chrono::steady_clock::time_point> _previous;
};

}  // namespace ampersand::core
