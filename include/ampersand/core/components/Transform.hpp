#pragma once

#include <ampersand/core/math/Vec2.hpp>

namespace ampersand::core::components {

using Position = math::Vec2f;
using Velocity = math::Vec2f;

struct Transform {
  Position position{};
  Velocity velocity{};
};

}  // namespace ampersand::core::components