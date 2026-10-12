#pragma once

#include <ampersand/render/Sprite.hpp>
#include <vector>

namespace ampersand::render {

/**
 * @brief Linear, looping animation over a sequence of sprite frames.
 *
 * Frame timing is advanced explicitly by calling update with elapsed seconds.
 */
class SpriteAnimation {
public:
    /**
     * @throws std::invalid_argument if frames is empty or frameDurationSeconds
     * is not finite and positive.
     */
    SpriteAnimation(std::vector<Sprite> frames, float frameDurationSeconds);

    /**
     * @brief Advances playback by the specified elapsed time.
     * @throws std::invalid_argument if deltaSeconds is negative or not finite.
     */
    void update(float deltaSeconds);

    /**
     * @brief Returns the frame currently selected for drawing.
     */
    [[nodiscard]] const Sprite& currentFrame() const;

    /**
     * @brief Restarts playback at the first frame.
     */
    void reset() noexcept;

private:
    std::vector<Sprite> frames_;
    double frameDurationSeconds_{};
    double elapsedSeconds_{};
};

}  // namespace ampersand::render
