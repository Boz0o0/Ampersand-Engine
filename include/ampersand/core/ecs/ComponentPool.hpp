#pragma once

#include <ampersand/core/ecs/Entity.hpp>
#include <cstdint>
#include <utility>
#include <vector>

namespace ampersand::core::ecs {

/**
 * @brief Type-independent base of every ComponentPool (to allow storing them in
 * a single array).
 */
class IComponentPool {
public:
    /** @brief Destroys the pool and its components. */
    virtual ~IComponentPool() = default;

    IComponentPool(const IComponentPool&) = delete;
    IComponentPool& operator=(const IComponentPool&) = delete;

protected:
    /** @brief Creates an empty pool. */
    IComponentPool() = default;
};

/**
 * @brief Packed storage (pool), used by every component type.
 *
 * `_data[i]` is the component owned by `_owners[i]`.
 *
 * @tparam T Component type.
 */
template <typename T>
class ComponentPool : public IComponentPool {
public:
    /**
     * @brief Appends a component at the end of the pool.
     *
     * @param entity Owner of the component.
     * @param component Component to store.
     * @return Position of the component in the pool.
     */
    [[nodiscard]] std::uint32_t push(Entity entity, T component) {
        _data.push_back(std::move(component));
        _owners.push_back(entity);
        return static_cast<std::uint32_t>(_data.size() - 1);
    }

    /**
     * @brief Accesses the component stored at a position.
     *
     * @param position Position (returned by push or stored in the World
     * position table (pos +1)) of the component in the pool.
     * @return Reference to the component.
     */
    [[nodiscard]] T& get(std::uint32_t position) { return _data[position]; }

private:
    std::vector<T> _data;
    std::vector<Entity> _owners;
};

}  // namespace ampersand::core::ecs
