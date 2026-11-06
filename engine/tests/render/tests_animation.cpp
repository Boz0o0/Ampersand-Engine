#include <gtest/gtest.h>

#include <ampersand/render/components/Animation.hpp>
#include <cstdint>
#include <stdexcept>

namespace {

using ampersand::core::math::Rect;
using ampersand::render::FrameGrid;
using ampersand::render::framesFromGrid;

// NOLINTNEXTLINE(readability-identifier-length)
void expectRect(const Rect<std::int32_t>& rect, std::int32_t x, std::int32_t y,
                std::int32_t width, std::int32_t height) {
    EXPECT_EQ(rect.x, x);
    EXPECT_EQ(rect.y, y);
    EXPECT_EQ(rect.width, width);
    EXPECT_EQ(rect.height, height);
}

}  // namespace

TEST(FramesFromGrid, CutsASingleRowLeftToRight) {
    const auto frames =
        framesFromGrid({.frameSize = {16, 24}, .columns = 3, .count = 3});

    ASSERT_EQ(frames.size(), 3U);
    expectRect(frames[0], 0, 0, 16, 24);
    expectRect(frames[1], 16, 0, 16, 24);
    expectRect(frames[2], 32, 0, 16, 24);
}

TEST(FramesFromGrid, WrapsToTheNextRowAfterTheLastColumn) {
    const auto frames =
        framesFromGrid({.frameSize = {16, 16}, .columns = 2, .count = 4});

    ASSERT_EQ(frames.size(), 4U);
    expectRect(frames[0], 0, 0, 16, 16);
    expectRect(frames[1], 16, 0, 16, 16);
    expectRect(frames[2], 0, 16, 16, 16);
    expectRect(frames[3], 16, 16, 16, 16);
}

TEST(FramesFromGrid, StopsInsideAnIncompleteLastRow) {
    const auto frames =
        framesFromGrid({.frameSize = {8, 8}, .columns = 4, .count = 5});

    ASSERT_EQ(frames.size(), 5U);
    expectRect(frames[3], 24, 0, 8, 8);
    expectRect(frames[4], 0, 8, 8, 8);
}

TEST(FramesFromGrid, SkipsTheGapBetweenFrames) {
    const auto frames = framesFromGrid(
        {.frameSize = {16, 16}, .columns = 2, .count = 3, .gap = {2, 4}});

    ASSERT_EQ(frames.size(), 3U);
    expectRect(frames[0], 0, 0, 16, 16);
    expectRect(frames[1], 18, 0, 16, 16);
    expectRect(frames[2], 0, 20, 16, 16);
}

TEST(FramesFromGrid, StartsAtTheOffset) {
    const auto frames = framesFromGrid({
        .frameSize = {16, 16},
        .columns = 2,
        .count = 3,
        .gap = {1, 1},
        .offset = {5, 10},
    });

    ASSERT_EQ(frames.size(), 3U);
    expectRect(frames[0], 5, 10, 16, 16);
    expectRect(frames[1], 22, 10, 16, 16);
    expectRect(frames[2], 5, 27, 16, 16);
}

TEST(FramesFromGrid, PutsEveryFrameInOneColumnWhenColumnsIsOne) {
    const auto frames =
        framesFromGrid({.frameSize = {10, 12}, .columns = 1, .count = 3});

    ASSERT_EQ(frames.size(), 3U);
    expectRect(frames[0], 0, 0, 10, 12);
    expectRect(frames[1], 0, 12, 10, 12);
    expectRect(frames[2], 0, 24, 10, 12);
}

TEST(FramesFromGrid, ThrowsWhenColumnsIsNotPositive) {
    EXPECT_THROW((void)framesFromGrid({.frameSize = {16, 16}, .columns = 0}),
                 std::invalid_argument);
    EXPECT_THROW((void)framesFromGrid({.frameSize = {16, 16}, .columns = -1}),
                 std::invalid_argument);
}

TEST(FramesFromGrid, ThrowsWhenCountIsNotPositive) {
    EXPECT_THROW((void)framesFromGrid({.frameSize = {16, 16}, .count = 0}),
                 std::invalid_argument);
    EXPECT_THROW((void)framesFromGrid({.frameSize = {16, 16}, .count = -3}),
                 std::invalid_argument);
}

TEST(FramesFromGrid, ThrowsWhenFrameSizeIsNotPositive) {
    EXPECT_THROW((void)framesFromGrid({.frameSize = {0, 16}}),
                 std::invalid_argument);
    EXPECT_THROW((void)framesFromGrid({.frameSize = {16, 0}}),
                 std::invalid_argument);
    EXPECT_THROW((void)framesFromGrid(FrameGrid{}), std::invalid_argument);
}
