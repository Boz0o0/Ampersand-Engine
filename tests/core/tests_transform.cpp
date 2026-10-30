#include <gtest/gtest.h>

#include <ampersand/core/components/Transform.hpp>

using ampersand::core::compose;
using ampersand::core::Transform;

/** @brief Tolerance for the positions computed with cos and sin. */
constexpr float kRotationTolerance = 1e-4F;

TEST(Transform, ComposeAddsThePositions) {
    Transform parent;
    parent.position = {100.0F, 50.0F, 1.0F};
    Transform child;
    child.position = {10.0F, 20.0F, 2.0F};

    const Transform result = compose(parent, child);

    EXPECT_FLOAT_EQ(result.position.x, 110.0F);
    EXPECT_FLOAT_EQ(result.position.y, 70.0F);
    EXPECT_FLOAT_EQ(result.position.z, 3.0F);
}

TEST(Transform, ComposeScalesTheChildOffsetAndMultipliesTheScales) {
    Transform parent;
    parent.position = {100.0F, 0.0F, 0.0F};
    parent.scale = {2.0F, 3.0F, 4.0F};
    Transform child;
    child.position = {10.0F, 10.0F, 0.0F};
    child.scale = {0.5F, 2.0F, 1.0F};

    const Transform result = compose(parent, child);

    EXPECT_FLOAT_EQ(result.position.x, 120.0F);
    EXPECT_FLOAT_EQ(result.position.y, 30.0F);
    EXPECT_FLOAT_EQ(result.scale.x, 1.0F);
    EXPECT_FLOAT_EQ(result.scale.y, 6.0F);
    EXPECT_FLOAT_EQ(result.scale.z, 4.0F);
}

TEST(Transform, ComposeRotatesTheChildAroundTheParent) {
    Transform parent;
    parent.position = {100.0F, 0.0F, 0.0F};
    parent.rotationDegrees = {0.0F, 0.0F, 90.0F};
    Transform child;
    child.position = {10.0F, 0.0F, 0.0F};
    child.rotationDegrees = {0.0F, 0.0F, 45.0F};

    const Transform result = compose(parent, child);

    EXPECT_NEAR(result.position.x, 100.0F, kRotationTolerance);
    EXPECT_NEAR(result.position.y, 10.0F, kRotationTolerance);
    EXPECT_FLOAT_EQ(result.rotationDegrees.z, 135.0F);
}

TEST(Transform, ComposeMeasuresTheChildFromTheParentOrigin) {
    Transform parent;
    parent.position = {100.0F, 100.0F, 0.0F};
    parent.origin = {16.0F, 8.0F, 0.0F};
    Transform child;
    child.position = {16.0F, 8.0F, 0.0F};
    child.origin = {4.0F, 2.0F, 0.0F};

    const Transform result = compose(parent, child);

    EXPECT_FLOAT_EQ(result.position.x, 100.0F);
    EXPECT_FLOAT_EQ(result.position.y, 100.0F);
    EXPECT_FLOAT_EQ(result.origin.x, 4.0F);
    EXPECT_FLOAT_EQ(result.origin.y, 2.0F);
}
