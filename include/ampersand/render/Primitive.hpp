#pragma once

#include <ampersand/core/math/Vec2.hpp>
#include <ampersand/render/Color.hpp>
#include <cstdint>

namespace ampersand::render {

/**
 * @brief Filled rectangle with an optional outline.
 */
struct Rectangle {
    core::math::Vec2f size;
    Color fill{};
    Color outline{};
    float outlineThickness{};
    std::int32_t layer{};
};

/**
 * @brief Filled circle with an optional outline.
 */
struct Circle {
    float radius{};
    Color fill{};
    Color outline{};
    float outlineThickness{};
    std::int32_t layer{};
};

/**
 * @brief A line segment.
 */
struct Line {
    core::math::Vec2f start;
    core::math::Vec2f end;
    Color color{};
    std::int32_t layer{};
};

}  // namespace ampersand::render
