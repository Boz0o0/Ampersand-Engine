#pragma once

#include <cstddef>
#include <cstdint>
#include <functional>

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

/** @brief Two entities are equal when they designate the same row version. */
[[nodiscard]] constexpr bool operator==(Entity lhs, Entity rhs) {
    return lhs.index == rhs.index && lhs.version == rhs.version;
}

/** @brief Negation of == */
[[nodiscard]] constexpr bool operator!=(Entity lhs, Entity rhs) {
    return !(lhs == rhs);
}

}  // namespace ampersand::core::ecs

/**
 * @brief Lets an Entity be the key of a std::unordered_map / unordered_set.
 *
 * Not meant to be called manually.
 *
 */
template <>
struct std::hash<ampersand::core::ecs::Entity> {
    [[nodiscard]] std::size_t operator()(
        ampersand::core::ecs::Entity entity) const noexcept {
        return std::hash<std::uint32_t>{}(entity.index);
    }
};
