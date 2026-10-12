#include <gtest/gtest.h>

#include <ampersand/core/components/Transform.hpp>
#include <ampersand/core/ecs/ECSExceptions.hpp>
#include <ampersand/core/ecs/ISystem.hpp>
#include <ampersand/core/ecs/World.hpp>
#include <ampersand/core/resources/Clock.hpp>
#include <ampersand/physics/components/Velocity.hpp>
#include <ampersand/physics/systems/MovementSystem.hpp>
#include <memory>

namespace {

using ampersand::core::Clock;
using ampersand::core::Transform;
using ampersand::core::ecs::Entity;
using ampersand::core::ecs::ISystem;
using ampersand::core::ecs::ResourceNotFound;
using ampersand::core::ecs::World;
using ampersand::physics::MovementSystem;
using ampersand::physics::Velocity;

Entity createMoving(World& world, float velocityX, float velocityY) {
    const Entity entity = world.create();
    world.add(entity, Transform{});
    world.add(entity, Velocity{.value = {velocityX, velocityY}});
    return entity;
}

}  // namespace

TEST(MovementSystem, MovesByVelocityTimesDt) {
    World world;
    world.addResource<Clock>().dt = 0.5F;
    const Entity ship = createMoving(world, 100.0F, -20.0F);

    MovementSystem{}.update(world);

    const auto& position = world.get<Transform>(ship).position;
    EXPECT_FLOAT_EQ(position.x, 50.0F);
    EXPECT_FLOAT_EQ(position.y, -10.0F);
}

TEST(MovementSystem, AddsToTheCurrentPosition) {
    World world;
    world.addResource<Clock>().dt = 1.0F;
    const Entity ship = createMoving(world, 10.0F, 0.0F);
    world.get<Transform>(ship).position.x = 5.0F;

    MovementSystem movement;
    movement.update(world);
    movement.update(world);

    EXPECT_FLOAT_EQ(world.get<Transform>(ship).position.x, 25.0F);
}

TEST(MovementSystem, KeepsZ) {
    World world;
    world.addResource<Clock>().dt = 1.0F;
    const Entity ship = createMoving(world, 10.0F, 10.0F);
    world.get<Transform>(ship).position.z = 7.0F;

    MovementSystem{}.update(world);

    EXPECT_FLOAT_EQ(world.get<Transform>(ship).position.z, 7.0F);
}

TEST(MovementSystem, ZeroDtDoesNotMove) {
    World world;
    world.addResource<Clock>();
    const Entity ship = createMoving(world, 100.0F, 100.0F);

    MovementSystem{}.update(world);

    const auto& position = world.get<Transform>(ship).position;
    EXPECT_FLOAT_EQ(position.x, 0.0F);
    EXPECT_FLOAT_EQ(position.y, 0.0F);
}

TEST(MovementSystem, IgnoresEntityWithoutTransform) {
    World world;
    world.addResource<Clock>().dt = 1.0F;
    const Entity ghost = world.create();
    world.add(ghost, Velocity{.value = {10.0F, 10.0F}});
    const Entity ship = createMoving(world, 10.0F, 0.0F);

    EXPECT_NO_THROW(MovementSystem{}.update(world));
    EXPECT_FALSE(world.has<Transform>(ghost));
    EXPECT_FLOAT_EQ(world.get<Transform>(ship).position.x, 10.0F);
}

TEST(MovementSystem, IgnoresEntityWithoutVelocity) {
    World world;
    world.addResource<Clock>().dt = 1.0F;
    const Entity wall = world.create();
    world.add(wall, Transform{});

    MovementSystem{}.update(world);

    EXPECT_FLOAT_EQ(world.get<Transform>(wall).position.x, 0.0F);
}

TEST(MovementSystem, ThrowsWithoutClock) {
    World world;
    createMoving(world, 10.0F, 10.0F);

    EXPECT_THROW(MovementSystem{}.update(world), ResourceNotFound);
}

TEST(MovementSystem, RunsThroughISystem) {
    World world;
    world.addResource<Clock>().dt = 1.0F;
    const Entity ship = createMoving(world, 3.0F, 4.0F);
    const std::unique_ptr<ISystem> system = std::make_unique<MovementSystem>();

    system->update(world);

    EXPECT_FLOAT_EQ(world.get<Transform>(ship).position.x, 3.0F);
    EXPECT_FLOAT_EQ(world.get<Transform>(ship).position.y, 4.0F);
}
