#include <gtest/gtest.h>

#include <ampersand/core/ecs/EventQueue.hpp>
#include <memory>
#include <vector>

using ampersand::core::ecs::EventQueue;

TEST(EventQueue, StartsEmpty) {
    const EventQueue<int> queue;

    EXPECT_TRUE(queue.read().empty());
}

TEST(EventQueue, ReadsTheEventsInTheOrderTheyWereSent) {
    EventQueue<int> queue;

    queue.send(3);
    queue.send(1);
    queue.send(2);

    EXPECT_EQ(queue.read(), (std::vector<int>{3, 1, 2}));
}

TEST(EventQueue, ClearRemovesEveryEvent) {
    EventQueue<int> queue;
    queue.send(1);
    queue.send(2);

    queue.clear();

    EXPECT_TRUE(queue.read().empty());
}

TEST(EventQueue, AcceptsEventsThatCanOnlyBeMoved) {
    EventQueue<std::unique_ptr<int>> queue;

    queue.send(std::make_unique<int>(4));

    ASSERT_EQ(queue.read().size(), 1U);
    EXPECT_EQ(*queue.read().front(), 4);
}
