#pragma once

#include <ampersand/core/math/Rect.hpp>
#include <cstdint>

namespace ampersand::render {

/**
 * @brief Identifier assigned to a loaded texture resource.
 */
using TextureId = std::uint32_t;

/**
 * @brief Describes a drawable sprite and the region of its texture to use.
 */
struct Sprite {
  TextureId texture{};
  ampersand::core::math::Rect<std::int32_t> source;
};

}  // namespace ampersand::render