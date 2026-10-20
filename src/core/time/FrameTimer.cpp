#include <algorithm>
#include <ampersand/core/time/FrameTimer.hpp>
#include <chrono>
#include <cmath>
#include <stdexcept>

namespace ampersand::core {

FrameTimer::FrameTimer(float maxDt) : _maxDt(maxDt) {
    if (!std::isfinite(maxDt) || maxDt <= 0.0F) {
        throw std::invalid_argument(
            "FrameTimer: maxDt must be finite and positive");
    }
}

float FrameTimer::tick() {
    const auto now = std::chrono::steady_clock::now();
    if (!_previous) {
        _previous = now;
        return 0.0F;
    }
    const std::chrono::duration<float> elapsed = now - *_previous;
    _previous = now;
    return std::min(elapsed.count(), _maxDt);
}

}  // namespace ampersand::core
