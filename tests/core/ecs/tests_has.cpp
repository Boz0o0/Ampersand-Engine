#include <gtest/gtest.h>

#include <ampersand/core/ecs/World.hpp>

namespace {

using ampersand::core::ecs::Entity;
using ampersand::core::ecs::World;

struct Position {};
struct Speed {};
struct Enemy {};

}  // namespace

TEST(Has, TrueWhenTheEntityHasEveryType) {
    World world;
    const auto entity = world.create();
    world.add(entity, Position{});
    world.add(entity, Speed{});

    EXPECT_TRUE(world.has<Position>(entity));
    EXPECT_TRUE((world.has<Position, Speed>(entity)));
}

TEST(Has, FalseWhenOneTypeIsMissing) {
    World world;
    const auto entity = world.create();
    world.add(entity, Position{});

    EXPECT_FALSE((world.has<Position, Speed>(entity)));
    EXPECT_FALSE((world.has<Enemy, Position>(entity)));
}

TEST(Has, FalseForADeadOrNullEntity) {
    World world;
    const auto entity = world.create();
    world.add(entity, Position{});
    world.add(entity, Speed{});
    world.destroy(entity);

    EXPECT_FALSE((world.has<Position, Speed>(entity)));
    EXPECT_FALSE((world.has<Position, Speed>(Entity{})));
}
