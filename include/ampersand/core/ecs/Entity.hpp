#pragma once

#include <cstdint>

namespace ampersand::core::ecs {

/**
 * @brief Handle to an entity: a row of the World plus the version of that row.
 */
struct Entity {
    /** @brief Row of the entity in the World. */
    std::uint32_t index{};
    /** @brief Version of the row when the entity was created. */
    std::uint32_t version{};
};

}  // namespace ampersand::core::ecs
