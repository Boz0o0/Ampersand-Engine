#pragma once

#include <ampersand/render/Color.hpp>
#include <ampersand/render/FontId.hpp>
#include <cstdint>
#include <string>

namespace ampersand::render {

/**
 * @brief Text drawn at the entity's Transform by the RenderSystem.
 */
struct Text {
    FontId font{};
    std::string value;
    /** @brief Character size, in pixels. */
    unsigned int size{24};
    Color color{.r = 255, .g = 255, .b = 255};
    std::int32_t layer{};
};

}  // namespace ampersand::render
