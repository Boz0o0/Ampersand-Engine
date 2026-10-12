#include <algorithm>
#include <ampersand/render/SpriteAnimation.hpp>
#include <cmath>
#include <stdexcept>
#include <utility>

namespace ampersand::render {

SpriteAnimation::SpriteAnimation(std::vector<Sprite> frames,
                                 float frameDurationSeconds)
    : frames_(std::move(frames)), frameDurationSeconds_(frameDurationSeconds) {
    if (this->frames_.empty() || !std::isfinite(this->frameDurationSeconds_) ||
        this->frameDurationSeconds_ <= 0.0) {
        throw std::invalid_argument(
            "SpriteAnimation requires frames and a finite positive frame "
            "duration");
    }
}

void SpriteAnimation::update(float deltaSeconds) {
    if (!std::isfinite(deltaSeconds) || deltaSeconds < 0.0F) {
        throw std::invalid_argument(
            "SpriteAnimation delta time must be finite and non-negative");
    }

    const double cycleDuration =
        this->frameDurationSeconds_ * static_cast<double>(this->frames_.size());
    this->elapsedSeconds_ =
        std::fmod(this->elapsedSeconds_ + deltaSeconds, cycleDuration);
}

const Sprite& SpriteAnimation::currentFrame() const {
    const auto frameIndex =
        std::min(static_cast<std::size_t>(this->elapsedSeconds_ /
                                          this->frameDurationSeconds_),
                 this->frames_.size() - 1);
    return this->frames_[frameIndex];
}

void SpriteAnimation::reset() noexcept { this->elapsedSeconds_ = 0.0; }

}  // namespace ampersand::render
