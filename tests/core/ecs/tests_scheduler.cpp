#include <gtest/gtest.h>

#include <ampersand/core/ecs/ECSExceptions.hpp>
#include <ampersand/core/ecs/EventQueue.hpp>
#include <ampersand/core/ecs/ISystem.hpp>
#include <ampersand/core/ecs/Scheduler.hpp>
#include <ampersand/core/ecs/World.hpp>
#include <ampersand/core/resources/Clock.hpp>
#include <limits>
#include <stdexcept>
#include <vector>

namespace {

using ampersand::core::Clock;
using ampersand::core::ecs::EventQueue;
using ampersand::core::ecs::ISystem;
using ampersand::core::ecs::ResourceAlreadyExists;
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

/** @brief Event of the tests. */
struct Hit {
    int value;
};

class SendSystem final : public ISystem {
public:
    explicit SendSystem(int value) : _value(value) {}

    void update(World& world) override {
        world.getResource<EventQueue<Hit>>().send({.value = _value});
    }

private:
    int _value;
};

class ReadSystem final : public ISystem {
public:
    void update(World& world) override {
        seen.clear();
        for (const Hit& hit : world.getResource<EventQueue<Hit>>().read()) {
            seen.push_back(hit.value);
        }
    }

    /** @brief Values of the events read during the last frame. */
    std::vector<int> seen;
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

TEST(Scheduler, AddsAnEmptyEventQueueToTheWorld) {
    World world;
    Scheduler scheduler(world);

    const EventQueue<Hit>& queue = scheduler.addEventQueue<Hit>();

    ASSERT_TRUE(world.hasResource<EventQueue<Hit>>());
    EXPECT_EQ(&queue, &world.getResource<EventQueue<Hit>>());
    EXPECT_TRUE(queue.read().empty());
}

TEST(Scheduler, ThrowsWhenTheEventQueueAlreadyExists) {
    World world;
    Scheduler scheduler(world);
    scheduler.addEventQueue<Hit>();

    EXPECT_THROW(scheduler.addEventQueue<Hit>(), ResourceAlreadyExists);
}

TEST(Scheduler, ALaterSystemReadsTheEventsOfTheFrame) {
    World world;
    Scheduler scheduler(world);
    scheduler.addEventQueue<Hit>();
    scheduler.add<SendSystem>(1);
    scheduler.add<SendSystem>(2);
    const ReadSystem& reader = scheduler.add<ReadSystem>();

    scheduler.update(0.0F);

    EXPECT_EQ(reader.seen, (std::vector<int>{1, 2}));
}

TEST(Scheduler, AnEarlierSystemNeverReadsTheEvents) {
    World world;
    Scheduler scheduler(world);
    scheduler.addEventQueue<Hit>();
    const ReadSystem& reader = scheduler.add<ReadSystem>();
    scheduler.add<SendSystem>(1);

    scheduler.update(0.0F);
    scheduler.update(0.0F);

    EXPECT_TRUE(reader.seen.empty());
}

TEST(Scheduler, EmptiesTheEventQueuesAtTheEndOfTheFrame) {
    World world;
    Scheduler scheduler(world);
    const EventQueue<Hit>& queue = scheduler.addEventQueue<Hit>();
    scheduler.add<SendSystem>(1);

    scheduler.update(0.0F);

    EXPECT_TRUE(queue.read().empty());
}

TEST(Scheduler, ReadsAnEventSentBetweenTwoFramesInTheNextOne) {
    World world;
    Scheduler scheduler(world);
    EventQueue<Hit>& queue = scheduler.addEventQueue<Hit>();
    const ReadSystem& reader = scheduler.add<ReadSystem>();

    queue.send({.value = 7});
    scheduler.update(0.0F);

    EXPECT_EQ(reader.seen, (std::vector<int>{7}));
    EXPECT_TRUE(queue.read().empty());
}

TEST(Scheduler, KeepsTheEventsWhenASystemThrows) {
    World world;
    Scheduler scheduler(world);
    const EventQueue<Hit>& queue = scheduler.addEventQueue<Hit>();
    scheduler.add<SendSystem>(1);
    scheduler.add<ThrowSystem>();

    EXPECT_THROW(scheduler.update(0.0F), std::runtime_error);
    EXPECT_EQ(queue.read().size(), 1U);
}
