#pragma once

#include <ampersand/core/ecs/ComponentId.hpp>
#include <ampersand/core/ecs/ComponentPool.hpp>
#include <ampersand/core/ecs/ECSConfig.hpp>
#include <ampersand/core/ecs/Entity.hpp>
#include <array>
#include <cstddef>
#include <cstdint>
#include <memory>
#include <queue>
#include <utility>
#include <vector>

namespace ampersand::core::ecs {

/**
 * @brief Owns the entities of one game and their components.
 *
 * Components are stored in one pool per type. Each entity has a row in
 * the position table with one cell per component type: the position of its
 * component in the pool (+1, so remove 1 when using it), or 0 when it has none
 *
 * A World is used by a single thread.
 */
class World {
public:
    /**
     * @brief Creates an empty World.
     *
     * @param maxEntities Maximum number of entities alive at once.
     */
    explicit World(std::uint32_t maxEntities = kDefaultMaxEntities);

    /**
     * @brief Creates a new entity with no component.
     *
     * @return Handle to the new entity.
     * @throws EntityLimitReached if maxEntities entities are already alive.
     */
    Entity create();

    /**
     * @brief Destroys an entity and frees its row for a later create().
     *
     * The version of the row is incremented, so every handle to the destroyed
     * entity stops being alive, even after the row is reused.
     *
     * @param entity Entity to destroy.
     * @throws InvalidEntity if the entity is dead, null or out of range.
     */
    void destroy(Entity entity);

    /**
     * @brief Checks whether a handle designates a live entity.
     *
     * @param entity Handle to check
     * @return true if the entity exists, false otherwise.
     */
    [[nodiscard]] bool alive(Entity entity) const;

    /**
     * @brief Attaches a component to an entity.
     *
     * @tparam T Component type, (deduced from the component arg).
     * @param entity Entity receiving the component.
     * @param component Component to attach, moved into the World.
     */
    template <typename T>
    void add(Entity entity, T component) {
        const auto position = poolOf<T>().push(entity, std::move(component));
        _rows[entity.index][componentId<T>()] = position + 1;  // 0 = absent
    }

    /**
     * @brief Accesses a component of an entity.
     *
     * @tparam T Component type.
     * @param entity Entity owning the component.
     * @return Reference to the component.
     */
    template <typename T>
    T& get(Entity entity) {
        const auto cell = _rows[entity.index][componentId<T>()];
        return poolOf<T>().get(cell - 1);  // cells store position + 1,
    }

    /**
     * @brief Checks whether an entity has a component of type T.
     *
     * @tparam T Component type.
     * @param entity Entity to check.
     * @return true if the entity has a component of type T, false otherwise.
     */
    template <typename T>
    [[nodiscard]] bool has(Entity entity) const {
        return alive(entity) && _rows[entity.index][componentId<T>()] != 0;
    }

    /**
     * @brief Removes a component of an entity.
     *
     * The pools last component moves into the freed position, and the row of
     * the entity owning it is updated.
     *
     * @tparam T Component type.
     * @param entity Entity owning the component.
     */
    template <typename T>
    void remove(Entity entity) {
        removeFromPool(entity, componentId<T>());
    }

private:
    /**
     * @brief Removes the component an entity has in a column (swap-and-pop)
     * and keeps the position table consistent.
     *
     * @param entity Entity owning the component.
     * @param column Column of the component type.
     */
    void removeFromPool(Entity entity, std::size_t column);

    /**
     * @brief One cell per component type: position in its pool + 1, or 0
     * when the entity has no component of that type.
     */
    using Row = std::array<std::uint32_t, kMaxComponents>;

    /**
     * @brief Returns the pool of component type T, creating it on first use.
     *
     * @tparam T Component type.
     * @return Reference to the pool of T.
     */
    template <typename T>
    ComponentPool<T>& poolOf() {
        auto& slot = _pools[componentId<T>()];
        if (!slot) {
            slot = std::make_unique<ComponentPool<T>>();
        }
        return static_cast<ComponentPool<T>&>(*slot);
    }

    std::uint32_t _maxEntities;
    std::vector<std::uint32_t> _versions;
    std::queue<std::uint32_t> _free;
    std::vector<Row> _rows;
    std::array<std::unique_ptr<IComponentPool>, kMaxComponents> _pools;
};

}  // namespace ampersand::core::ecs
