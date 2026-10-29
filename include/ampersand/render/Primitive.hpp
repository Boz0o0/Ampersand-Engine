#pragma once

#include <ampersand/core/math/Vec2.hpp>
#include <ampersand/render/Color.hpp>
#include <cstdint>

namespace ampersand::render {

/**
 * @brief Filled rectangle with an optional outline, drawn from the local
 * (0, 0) of its Transform.
 *
 * A component like Sprite: the RenderSystem draws it at the world transform
 * of its entity. It can also be drawn by hand with IRenderer::draw.
 */
struct Rectangle {
    core::math::Vec2f size;
    Color fill{};
    Color outline{};
    float outlineThickness{};
    std::int32_t layer{0};
};

/**
 * @brief Filled circle with an optional outline, drawn in the box of its
 * diameter from the local (0, 0) of its Transform (an origin of (radius,
 * radius) centers it on the position).
 *
 * A component like Sprite: the RenderSystem draws it at the world transform
 * of its entity. It can also be drawn by hand with IRenderer::draw.
 */
struct Circle {
    float radius{};
    Color fill{};
    Color outline{};
    float outlineThickness{};
    std::int32_t layer{0};
};

/**
 * @brief A line segment, from and to local points of its Transform.
 *
 * A component like Sprite: the RenderSystem draws it at the world transform
 * of its entity. It can also be drawn by hand with IRenderer::draw.
 */
struct Line {
    core::math::Vec2f from;
    core::math::Vec2f to;
    Color color{};
    std::int32_t layer{0};
};

}  // namespace ampersand::render
