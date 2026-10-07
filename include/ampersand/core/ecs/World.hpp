#pragma once

#include <ampersand/core/ecs/ECSConfig.hpp>
#include <ampersand/core/ecs/Entity.hpp>
#include <cstdint>
#include <queue>
#include <vector>

namespace ampersand::core::ecs {

/**
 * @brief Owns the entities of one game (and, later, their components).
 *
 * A World is used by a single thread. Several Worlds can live side by side,
 * e.g. one per game instance on a server.
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
     * @param entity Handle to check; the null entity is never alive.
     * @return true if the entity exists, false otherwise. Never throws.
     */
    [[nodiscard]] bool alive(Entity entity) const;

private:
    std::uint32_t _maxEntities;
    std::vector<std::uint32_t> _versions;
    std::queue<std::uint32_t> _free;
};

}  // namespace ampersand::core::ecs
