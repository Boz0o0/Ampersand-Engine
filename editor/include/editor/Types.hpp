#pragma once

#include <nlohmann/json.hpp>

/**
 * @file Types.hpp
 * @brief Default JSON of the component and resource types the editor can add.
 */

namespace ampersand::editor {

/** @brief Transform at the origin, with no rotation and a scale of 1. */
inline nlohmann::ordered_json defaultTransform() {
    const auto vector = [](float value) {
        return nlohmann::ordered_json{{"x", value}, {"y", value}, {"z", value}};
    };
    return {
        {"position", vector(0.0F)},
        {"scale", vector(1.0F)},
        {"rotationDegrees", vector(0.0F)},
        {"origin", vector(0.0F)},
    };
}

}  // namespace ampersand::editor
