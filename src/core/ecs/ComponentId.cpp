#include <ampersand/core/ecs/ComponentId.hpp>
#include <atomic>
#include <cstddef>

namespace ampersand::core::ecs {

namespace {

/** @brief Next column to hand out. (next component id) */
std::atomic<std::size_t> nextColumn{0};

}  // namespace

std::size_t allocateComponentId() {
    const std::size_t column = nextColumn.fetch_add(1);
    return column;
}

}  // namespace ampersand::core::ecs
