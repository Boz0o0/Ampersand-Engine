#pragma once

#include <ampersand/core/math/Vec2.hpp>
#include <cstdint>
#include <variant>

namespace ampersand::render {

/**
 * @brief Enumeration of supported keyboard keys for window input events.
 */
enum class Key : std::uint8_t { Up, Down, Left, Right, Space, Escape, Unknown };

/**
 * @brief Event triggered when the user closes the application window.
 */
struct Closed {};

/**
 * @brief Event triggered when the window size changes.
 */
struct Resized {
  ampersand::core::math::Vec2u size;
};

/**
 * @brief Event triggered when a key is pressed.
 */
struct KeyPressed {
  Key key;
};

/**
 * @brief Event triggered when a key is released.
 */
struct KeyReleased {
  Key key;
};

/**
 * @brief Union of all supported rendering and input events.
 */
using Event = std::variant<Closed, Resized, KeyPressed, KeyReleased>;

}  // namespace ampersand::render