#include <gtest/gtest.h>

#include <ampersand/core/ecs/ISystem.hpp>
#include <ampersand/core/ecs/Scheduler.hpp>
#include <ampersand/core/ecs/World.hpp>
#include <ampersand/core/resources/Clock.hpp>
#include <limits>
#include <stdexcept>
#include <vector>

namespace {

using ampersand::core::Clock;
using ampersand::core::ecs::ISystem;
using ampersand::core::ecs::Scheduler;
using ampersand::core::ecs::World;

/** @brief Ids of the systems, in the order they ran. */
struct Calls {
    std::vector<int> ids;
};

class RecordSystem final : public ISystem {
public:
    explicit RecordSystem(int systemId) : _id(systemId) {}

    void update(World& world) override {
        world.getResource<Calls>().ids.push_back(_id);
    }

private:
    int _id;
};

class ClockSystem final : public ISystem {
public:
    void update(World& world) override { seen = world.getResource<Clock>(); }

    Clock seen;
};

class ThrowSystem final : public ISystem {
public:
    void update(World& /*world*/) override {
        throw std::runtime_error("system failed");
    }
};

}  // namespace

TEST(Scheduler, RunsSystemsInTheOrderTheyWereAdded) {
    World world;
    world.addResource<Calls>();
    Scheduler scheduler(world);
    scheduler.add<RecordSystem>(2);
    scheduler.add<RecordSystem>(0);
    scheduler.add<RecordSystem>(1);

    scheduler.update(0.0F);
    scheduler.update(0.0F);

    EXPECT_EQ(world.getResource<Calls>().ids,
              (std::vector<int>{2, 0, 1, 2, 0, 1}));
}

TEST(Scheduler, AddsAClockToTheWorld) {
    World world;

    const Scheduler scheduler(world);

    ASSERT_TRUE(world.hasResource<Clock>());
    EXPECT_FLOAT_EQ(world.getResource<Clock>().dt, 0.0F);
    EXPECT_FLOAT_EQ(world.getResource<Clock>().elapsed, 0.0F);
}

TEST(Scheduler, KeepsTheClockOfTheWorld) {
    World world;
    world.addResource<Clock>().elapsed = 10.0F;
    Scheduler scheduler(world);

    scheduler.update(0.5F);

    EXPECT_FLOAT_EQ(world.getResource<Clock>().elapsed, 10.5F);
}

TEST(Scheduler, SetsTheClockBeforeTheSystems) {
    World world;
    Scheduler scheduler(world);
    const ClockSystem& system = scheduler.add<ClockSystem>();

    scheduler.update(0.25F);
    scheduler.update(0.5F);

    EXPECT_FLOAT_EQ(system.seen.dt, 0.5F);
    EXPECT_FLOAT_EQ(system.seen.elapsed, 0.75F);
}

TEST(Scheduler, RefusesANegativeOrNotFiniteDt) {
    World world;
    Scheduler scheduler(world);

    EXPECT_THROW(scheduler.update(-0.1F), std::invalid_argument);
    EXPECT_THROW(scheduler.update(std::numeric_limits<float>::infinity()),
                 std::invalid_argument);
    EXPECT_THROW(scheduler.update(std::numeric_limits<float>::quiet_NaN()),
                 std::invalid_argument);
    EXPECT_FLOAT_EQ(world.getResource<Clock>().elapsed, 0.0F);
}

TEST(Scheduler, StopsTheFrameWhenASystemThrows) {
    World world;
    world.addResource<Calls>();
    Scheduler scheduler(world);
    scheduler.add<RecordSystem>(0);
    scheduler.add<ThrowSystem>();
    scheduler.add<RecordSystem>(1);

    EXPECT_THROW(scheduler.update(0.0F), std::runtime_error);
    EXPECT_EQ(world.getResource<Calls>().ids, (std::vector<int>{0}));
}
