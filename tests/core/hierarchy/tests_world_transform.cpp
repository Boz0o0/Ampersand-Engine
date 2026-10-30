#include <gtest/gtest.h>

#include <ampersand/core/components/Transform.hpp>
#include <ampersand/core/ecs/World.hpp>
#include <ampersand/core/hierarchy/WorldTransform.hpp>

using ampersand::core::Transform;
using ampersand::core::worldTransform;
using ampersand::core::ecs::World;

TEST(WorldTransform, RootKeepsItsLocalTransform) {
    World world;
    const auto root = world.create();
    Transform transform;
    transform.position = {100.0F, 50.0F, 1.0F};
    world.add(root, transform);

    const Transform result = worldTransform(world, root);

    EXPECT_FLOAT_EQ(result.position.x, 100.0F);
    EXPECT_FLOAT_EQ(result.position.y, 50.0F);
    EXPECT_FLOAT_EQ(result.position.z, 1.0F);
}

TEST(WorldTransform, ComposesThreeLevels) {
    World world;
    const auto ship = world.create();
    const auto turret = world.create();
    const auto cannon = world.create();
    world.setParent(turret, ship);
    world.setParent(cannon, turret);
    Transform shipTransform;
    shipTransform.position = {100.0F, 0.0F, 0.0F};
    shipTransform.scale = {2.0F, 2.0F, 1.0F};
    Transform turretTransform;
    turretTransform.position = {10.0F, 5.0F, 0.0F};
    Transform cannonTransform;
    cannonTransform.position = {3.0F, 0.0F, 0.0F};
    world.add(ship, shipTransform);
    world.add(turret, turretTransform);
    world.add(cannon, cannonTransform);

    const Transform result = worldTransform(world, cannon);

    EXPECT_FLOAT_EQ(result.position.x, 126.0F);
    EXPECT_FLOAT_EQ(result.position.y, 10.0F);
    EXPECT_FLOAT_EQ(result.scale.x, 2.0F);
}

TEST(WorldTransform, SkipsAParentWithoutTransform) {
    World world;
    const auto ship = world.create();
    const auto group = world.create();
    const auto turret = world.create();
    world.setParent(group, ship);
    world.setParent(turret, group);
    Transform shipTransform;
    shipTransform.position = {100.0F, 50.0F, 0.0F};
    Transform turretTransform;
    turretTransform.position = {10.0F, 0.0F, 0.0F};
    world.add(ship, shipTransform);
    world.add(turret, turretTransform);

    const Transform result = worldTransform(world, turret);

    EXPECT_FLOAT_EQ(result.position.x, 110.0F);
    EXPECT_FLOAT_EQ(result.position.y, 50.0F);
}
