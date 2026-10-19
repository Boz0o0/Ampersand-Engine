#pragma once

#include <cstdint>

namespace ampersand::render {

/**
 * @brief Plays frames of a spritesheet on the entity's Sprite, driven by the
 * AnimationSystem.
 *
 * The texture is a grid of frames of the same size, numbered from left to
 * right then top to bottom (0 = top left). The animation plays `count` frames
 * starting at frame `first`, and the AnimationSystem writes the current one
 * into Sprite::source. Set `elapsed` back to 0 to restart from the first
 * frame.
 */
struct Animation {
    /** @brief Size of one frame, in pixels. */
    std::int32_t frameWidth{};
    std::int32_t frameHeight{};
    /** @brief Frames on one row of the texture (must be positive). */
    std::int32_t columns{1};
    /** @brief Number of frames played (0 = nothing is played). */
    std::int32_t count{};
    /** @brief Number of the first frame played in the grid (0 = top left).
     */
    std::int32_t first{0};
    /** @brief Seconds each frame stays on screen (must be positive). */
    float frameDuration{0.1F};
    /** @brief Starts over after the last frame, or stays on it if false. */
    bool loop{true};
    /** @brief Seconds since the first frame, advanced by the
     * AnimationSystem. */
    float elapsed{};
};

/**
 * @brief Checks if a non-looping Animation reached its last frame
 *
 * @param animation Animation to check.
 * @return true if `loop` is false and the whole animation was played. A
 * looping animation never finishes.
 */
[[nodiscard]] bool isFinished(const Animation& animation);

}  // namespace ampersand::render
