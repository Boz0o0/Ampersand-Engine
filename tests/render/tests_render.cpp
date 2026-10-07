#include <gtest/gtest.h>

#include <ampersand/render/SfmlRenderer.hpp>
#include <ampersand/render/SpriteAnimation.hpp>
#include <limits>
#include <stdexcept>
#include <vector>

namespace {

std::vector<ampersand::render::Sprite> makeFrames() {
  using ampersand::core::math::Rect;
  using ampersand::render::Sprite;
  return {
      {1, Rect<std::int32_t>{0, 0, 16, 16}},
      {1, Rect<std::int32_t>{16, 0, 16, 16}},
      {1, Rect<std::int32_t>{32, 0, 16, 16}},
  };
}

}  // namespace

TEST(SpriteAnimation, AdvancesFramesAndLoops) {
  ampersand::render::SpriteAnimation animation(makeFrames(), 0.5F);
  EXPECT_EQ(animation.currentFrame().source.x, 0);

  animation.update(0.5F);
  EXPECT_EQ(animation.currentFrame().source.x, 16);

  animation.update(1.0F);
  EXPECT_EQ(animation.currentFrame().source.x, 0);
}

TEST(SpriteAnimation, ResetReturnsToFirstFrame) {
  ampersand::render::SpriteAnimation animation(makeFrames(), 0.5F);
  animation.update(1.0F);
  ASSERT_EQ(animation.currentFrame().source.x, 32);

  animation.reset();
  EXPECT_EQ(animation.currentFrame().source.x, 0);
}

TEST(SpriteAnimation, AdvancesByFractionalElapsedTime) {
  ampersand::render::SpriteAnimation animation(makeFrames(), 0.5F);

  animation.update(0.25F);
  EXPECT_EQ(animation.currentFrame().source.x, 0);

  animation.update(0.25F);
  EXPECT_EQ(animation.currentFrame().source.x, 16);

  animation.update(0.5F);
  EXPECT_EQ(animation.currentFrame().source.x, 32);

  animation.update(0.5F);
  EXPECT_EQ(animation.currentFrame().source.x, 0);
}

TEST(SpriteAnimation, RejectsInvalidConfigurationAndDelta) {
  EXPECT_THROW(ampersand::render::SpriteAnimation({}, 0.5F),
               std::invalid_argument);
  EXPECT_THROW(ampersand::render::SpriteAnimation(makeFrames(), 0.0F),
               std::invalid_argument);

  ampersand::render::SpriteAnimation animation(makeFrames(), 0.5F);
  EXPECT_THROW(animation.update(-0.1F), std::invalid_argument);
  EXPECT_THROW(animation.update(std::numeric_limits<float>::infinity()),
               std::invalid_argument);
}
