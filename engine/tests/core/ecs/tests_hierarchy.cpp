#include <gtest/gtest.h>

#include <ampersand/core/ecs/ECSExceptions.hpp>
#include <ampersand/core/ecs/World.hpp>
#include <vector>

using ampersand::core::ecs::Entity;
using ampersand::core::ecs::InvalidEntity;
using ampersand::core::ecs::World;

TEST(Hierarchy, NewEntityIsARootWithoutChildren) {
    World world;
    const auto entity = world.create();

    EXPECT_EQ(world.parent(entity), Entity{});
    EXPECT_TRUE(world.children(entity).empty());
}

TEST(Hierarchy, SetParentLinksBothSides) {
    World world;
    const auto parent = world.create();
    const auto child = world.create();

    world.setParent(child, parent);

    EXPECT_EQ(world.parent(child), parent);
    EXPECT_EQ(world.children(parent), (std::vector<Entity>{child}));
}

TEST(Hierarchy, SetParentAgainKeepsASingleChild) {
    World world;
    const auto parent = world.create();
    const auto child = world.create();

    world.setParent(child, parent);
    world.setParent(child, parent);

    EXPECT_EQ(world.children(parent).size(), 1U);
}

TEST(Hierarchy, ReparentRemovesTheChildFromTheOldParent) {
    World world;
    const auto oldParent = world.create();
    const auto newParent = world.create();
    const auto child = world.create();
    world.setParent(child, oldParent);

    world.setParent(child, newParent);

    EXPECT_TRUE(world.children(oldParent).empty());
    EXPECT_EQ(world.children(newParent), (std::vector<Entity>{child}));
    EXPECT_EQ(world.parent(child), newParent);
}

TEST(Hierarchy, NullParentMakesTheChildARoot) {
    World world;
    const auto parent = world.create();
    const auto child = world.create();
    world.setParent(child, parent);

    world.setParent(child, Entity{});

    EXPECT_EQ(world.parent(child), Entity{});
    EXPECT_TRUE(world.children(parent).empty());
}

TEST(Hierarchy, ThrowsOnADeadEntity) {
    World world;
    const auto alive = world.create();
    const auto dead = world.create();
    world.destroy(dead);

    EXPECT_THROW(world.setParent(dead, alive), InvalidEntity);
    EXPECT_THROW(world.setParent(alive, dead), InvalidEntity);
    EXPECT_THROW((void)world.parent(dead), InvalidEntity);
    EXPECT_THROW((void)world.children(dead), InvalidEntity);
}

TEST(Hierarchy, ReusedRowsStartWithoutLinks) {
    World world{2};
    const auto parent = world.create();
    const auto child = world.create();
    world.setParent(child, parent);
    world.destroy(parent);

    const auto first = world.create();
    const auto second = world.create();

    EXPECT_EQ(world.parent(first), Entity{});
    EXPECT_EQ(world.parent(second), Entity{});
    EXPECT_TRUE(world.children(first).empty());
    EXPECT_TRUE(world.children(second).empty());
}

TEST(Hierarchy, DestroyDestroysTheWholeSubtree) {
    World world;
    const auto root = world.create();
    const auto middle = world.create();
    const auto leaf = world.create();
    const auto other = world.create();
    world.setParent(middle, root);
    world.setParent(leaf, middle);

    world.destroy(root);

    EXPECT_FALSE(world.alive(root));
    EXPECT_FALSE(world.alive(middle));
    EXPECT_FALSE(world.alive(leaf));
    EXPECT_TRUE(world.alive(other));
}

TEST(Hierarchy, DestroyRemovesTheChildFromItsParent) {
    World world;
    const auto parent = world.create();
    const auto first = world.create();
    const auto second = world.create();
    world.setParent(first, parent);
    world.setParent(second, parent);

    world.destroy(first);

    EXPECT_TRUE(world.alive(parent));
    EXPECT_EQ(world.children(parent), (std::vector<Entity>{second}));
}
