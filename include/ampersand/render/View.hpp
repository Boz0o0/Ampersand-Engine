#pragma once

#include <ampersand/core/math/Vec2.hpp>

namespace ampersand::render {

/**
 * @brief Camera view in world coordinates.
 */
struct View {
    core::math::Vec2f center;
    core::math::Vec2f size;
};

}  // namespace ampersand::render
