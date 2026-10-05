#pragma once

#include <cstdint>

namespace ampersand::core::math {

template <typename T>
struct Rect {
  T x{};
  T y{};
  T width{};
  T height{};

  constexpr Rect() = default;

  // NOLINTNEXTLINE(bugprone-easily-swappable-parameters)
  constexpr Rect(T xValue, T yValue, T widthValue, T heightValue)
      : x(xValue), y(yValue), width(widthValue), height(heightValue) {}
};

}  // namespace ampersand::core::math