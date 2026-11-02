#include <gtest/gtest.h>

#include <ampersand/core/ecs/CommandQueue.hpp>
#include <ampersand/core/ecs/Entity.hpp>
#include <ampersand/core/ecs/World.hpp>

namespace {

using ampersand::core::ecs::CommandQueue;
using ampersand::core::ecs::Entity;
using ampersand::core::ecs::World;

}  // namespace

TEST(CommandQueue, KeepsTheEntityAliveUntilApplied) {
    World world;
    CommandQueue commands;
    const Entity entity = world.create();

    commands.destroy(entity);

    EXPECT_TRUE(world.alive(entity));
}

TEST(CommandQueue, ApplyDestroysTheEntities) {
    World world;
    CommandQueue commands;
    const Entity first = world.create();
    const Entity second = world.create();
    const Entity kept = world.create();
    commands.destroy(first);
    commands.destroy(second);

    commands.apply(world);

    EXPECT_FALSE(world.alive(first));
    EXPECT_FALSE(world.alive(second));
    EXPECT_TRUE(world.alive(kept));
}

TEST(CommandQueue, ApplyEmptiesTheQueue) {
    World world;
    CommandQueue commands;
    commands.destroy(world.create());
    commands.apply(world);
    const Entity created = world.create();  // may reuse the destroyed row

    commands.apply(world);

    EXPECT_TRUE(world.alive(created));
}

TEST(CommandQueue, DestroysAnEntityRecordedTwiceOnce) {
    World world;
    CommandQueue commands;
    const Entity entity = world.create();
    commands.destroy(entity);
    commands.destroy(entity);

    EXPECT_NO_THROW(commands.apply(world));
    EXPECT_FALSE(world.alive(entity));
}

TEST(CommandQueue, SkipsAnEntityAlreadyDead) {
    World world;
    CommandQueue commands;
    const Entity entity = world.create();
    commands.destroy(entity);
    world.destroy(entity);
    const Entity reused = world.create();  // may reuse the row of entity

    EXPECT_NO_THROW(commands.apply(world));
    EXPECT_TRUE(world.alive(reused));
}
