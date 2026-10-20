#include <gtest/gtest.h>

#include <ampersand/core/time/FrameTimer.hpp>
#include <chrono>
#include <limits>
#include <stdexcept>
#include <thread>

using ampersand::core::FrameTimer;

TEST(FrameTimer, FirstTickIsZero) {
    FrameTimer timer(0.1F);

    EXPECT_FLOAT_EQ(timer.tick(), 0.0F);
}

TEST(FrameTimer, TickIsTheTimeSinceThePreviousOne) {
    FrameTimer timer(1.0F);
    timer.tick();

    std::this_thread::sleep_for(std::chrono::milliseconds(20));
    const float deltaTime = timer.tick();

    EXPECT_GE(deltaTime, 0.02F);
    EXPECT_LT(deltaTime, 1.0F);
}

TEST(FrameTimer, TickIsAtMostMaxDt) {
    FrameTimer timer(0.001F);
    timer.tick();

    std::this_thread::sleep_for(std::chrono::milliseconds(20));

    EXPECT_FLOAT_EQ(timer.tick(), 0.001F);
}

TEST(FrameTimer, MaxDtIsTheDefaultOneWhenNotGiven) {
    FrameTimer timer;
    timer.tick();

    std::this_thread::sleep_for(std::chrono::milliseconds(150));

    EXPECT_FLOAT_EQ(timer.tick(), FrameTimer::kDefaultMaxDt);
}

TEST(FrameTimer, RefusesAMaxDtThatIsNotPositiveOrNotFinite) {
    EXPECT_THROW(FrameTimer{0.0F}, std::invalid_argument);
    EXPECT_THROW(FrameTimer{-1.0F}, std::invalid_argument);
    EXPECT_THROW(FrameTimer{std::numeric_limits<float>::infinity()},
                 std::invalid_argument);
}
