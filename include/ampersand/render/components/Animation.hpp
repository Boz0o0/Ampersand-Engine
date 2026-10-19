#pragma once

#include <ampersand/core/math/Rect.hpp>
#include <ampersand/core/math/Vec2.hpp>
#include <cstdint>
#include <vector>

namespace ampersand::render {

/**
 * @brief Plays a list of frames on the entity's Sprite, driven by the
 * AnimationSystem.
 *
 * Each frame is a region of the Sprite's texture (like Sprite::source). The
 * AnimationSystem writes the current frame into Sprite::source. Set
 * `elapsed` back to 0 to restart from the first frame.
 */
struct Animation {
    std::vector<core::math::Rect<std::int32_t>> frames;
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

/**
 * @brief Layout of same-sized frames on a spritesheet, read by framesFromGrid.
 *
 * Frames are read left to right, then top to bottom, starting at `offset`.
 */
struct FrameGrid {
    /** @brief Width and height of one frame, in pixels. */
    core::math::Vec2<std::int32_t> frameSize;
    /** @brief Frames per row. */
    std::int32_t columns{1};
    /** @brief Total number of frames (the last row can be incomplete). */
    std::int32_t count{1};
    /** @brief Empty pixels between two frames, horizontally and vertically. */
    core::math::Vec2<std::int32_t> gap;
    /** @brief Top-left corner of the first frame on the texture. */
    core::math::Vec2<std::int32_t> offset;
};

/**
 * @brief Cuts a spritesheet grid into frames for Animation::frames
 *
 * @param grid Layout of the frames on the texture.
 * @return `count` rects, in reading order. Empty if `columns` or `count` is
 * not positive.
 */
[[nodiscard]] std::vector<core::math::Rect<std::int32_t>> framesFromGrid(
    const FrameGrid& grid);

}  // namespace ampersand::render
