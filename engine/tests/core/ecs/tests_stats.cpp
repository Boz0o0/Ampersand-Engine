#include <gtest/gtest.h>

#include <ampersand/core/ecs/ISystem.hpp>
#include <ampersand/core/ecs/Scheduler.hpp>
#include <ampersand/core/ecs/World.hpp>
#include <ampersand/core/resources/Stats.hpp>
#include <chrono>
#include <cstddef>
#include <thread>

namespace {

using ampersand::core::Stats;
using ampersand::core::ecs::ISystem;
using ampersand::core::ecs::Scheduler;
using ampersand::core::ecs::World;
using namespace std::chrono_literals;

/** @brief A system that takes 20 ms. */
class SlowSystem final : public ISystem {
public:
    void update(World& /*world*/) override {
        std::this_thread::sleep_for(20ms);
    }
};

}  // namespace

TEST(Stats, NotAddedByTheScheduler) {
    World world;
    Scheduler scheduler(world);

    scheduler.update(0.0F);

    EXPECT_FALSE(world.hasResource<Stats>());
}

TEST(Stats, CountsTheEntitiesAtTheEndOfTheFrame) {
    World world;
    Scheduler scheduler(world);
    const auto& stats = world.addResource<Stats>();
    world.create();
    world.destroy(world.create());

    scheduler.update(0.0F);

    EXPECT_EQ(stats.entities, 1U);
}

TEST(Stats, MeasuresTheTimeSpentInTheUpdate) {
    World world;
    Scheduler scheduler(world);
    const auto& stats = world.addResource<Stats>();
    scheduler.add<SlowSystem>();

    scheduler.update(0.0F);

    EXPECT_GE(stats.updateMs, 20.0F);
}

TEST(Stats, NoFrameTimeOnTheFirstFrame) {
    World world;
    Scheduler scheduler(world);
    const auto& stats = world.addResource<Stats>();

    scheduler.update(0.0F);

    EXPECT_EQ(stats.frameMs, 0.0F);
    EXPECT_EQ(stats.history.back(), 0.0F);
}

TEST(Stats, MeasuresTheRealTimeBetweenTwoFrames) {
    World world;
    Scheduler scheduler(world);
    const auto& stats = world.addResource<Stats>();

    scheduler.update(0.0F);
    std::this_thread::sleep_for(20ms);
    scheduler.update(0.0F);  // the dt given is not used

    EXPECT_GE(stats.frameMs, 20.0F);
    EXPECT_EQ(stats.history.back(), stats.frameMs);
}

TEST(Stats, HistoryDropsTheOldestFrameWhenFull) {
    World world;
    Scheduler scheduler(world);
    const auto& stats = world.addResource<Stats>();

    scheduler.update(0.0F);  // the first frame is not in the history
    std::this_thread::sleep_for(20ms);
    scheduler.update(0.0F);  // the slow frame, the oldest one
    for (std::size_t frame = 1; frame < Stats::kHistory; ++frame) {
        scheduler.update(0.0F);
    }
    EXPECT_GE(stats.history.front(), 20.0F);

    scheduler.update(0.0F);

    EXPECT_LT(stats.history.front(), 20.0F);  // the slow frame is gone
    EXPECT_EQ(stats.history.back(), stats.frameMs);
}

TEST(Stats, FpsAfterOneSecond) {
    World world;
    Scheduler scheduler(world);
    const auto& stats = world.addResource<Stats>();

    scheduler.update(0.0F);
    for (int frame = 0; frame < 5; ++frame) {
        std::this_thread::sleep_for(100ms);
        scheduler.update(0.0F);
    }
    EXPECT_EQ(stats.fps, 0.0F);  // about 0.5 s: not computed yet

    for (int frame = 0; frame < 10 && stats.fps == 0.0F; ++frame) {
        std::this_thread::sleep_for(100ms);
        scheduler.update(0.0F);
    }

    EXPECT_GT(stats.fps, 0.0F);
    EXPECT_LE(stats.fps, 10.0F);  // at least 100 ms per frame
}

TEST(Stats, KeepsTheValuesOfTheGame) {
    World world;
    Scheduler scheduler(world);
    auto& stats = world.addResource<Stats>();
    stats.values["game.ping"] = 42.0F;

    scheduler.update(0.0F);

    EXPECT_EQ(stats.values.at("game.ping"), 42.0F);
}
