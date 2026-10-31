#include <gtest/gtest.h>

#include <ampersand/core/ecs/World.hpp>

namespace {

using ampersand::core::ecs::World;

}  // namespace

TEST(EntityCount, ZeroForAnEmptyWorld) {
    const World world;

    EXPECT_EQ(world.entityCount(), 0U);
}

TEST(EntityCount, CountsTheCreatedEntities) {
    World world;
    world.create();
    world.create();

    EXPECT_EQ(world.entityCount(), 2U);
}

TEST(EntityCount, ForgetsTheDestroyedEntities) {
    World world;
    const auto first = world.create();
    world.create();
    world.destroy(first);

    EXPECT_EQ(world.entityCount(), 1U);
}

TEST(EntityCount, CountsTheChildrenAndForgetsThemWithTheirParent) {
    World world;
    const auto parent = world.create();
    const auto child = world.create();
    world.setParent(child, parent);
    EXPECT_EQ(world.entityCount(), 2U);

    world.destroy(parent);

    EXPECT_EQ(world.entityCount(), 0U);
}

TEST(EntityCount, CountsAReusedRowOnce) {
    World world;
    world.destroy(world.create());
    world.create();  // takes the freed row

    EXPECT_EQ(world.entityCount(), 1U);
}
