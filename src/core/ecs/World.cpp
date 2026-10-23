#include <ampersand/core/ecs/ECSExceptions.hpp>
#include <ampersand/core/ecs/World.hpp>
#include <cstddef>
#include <string>
#include <utility>
#include <vector>

namespace ampersand::core::ecs {

namespace {

std::string describe(Entity entity) {
    return "{" + std::to_string(entity.index) + ", " +
           std::to_string(entity.version) + "}";
}

}  // namespace

World::World(std::uint32_t maxEntities) : _maxEntities(maxEntities) {
    _versions.reserve(maxEntities);
    _rows.reserve(maxEntities);
    _parents.reserve(maxEntities);
    _children.reserve(maxEntities);
}

Entity World::create() {
    if (!_free.empty()) {
        const auto index = _free.front();
        _free.pop();
        return {.index = index, .version = _versions[index]};
    }
    if (_versions.size() >= _maxEntities) {
        throw EntityLimitReached(std::to_string(_maxEntities) + " entities");
    }
    const auto index = static_cast<std::uint32_t>(_versions.size());
    _versions.push_back(1);
    _rows.emplace_back();  // every cell is 0
    _parents.emplace_back();
    _children.emplace_back();
    return {.index = index, .version = _versions[index]};
}

void World::destroy(Entity entity) {
    requireAlive(entity, "destroy");
    setParent(entity, Entity{});
    const auto orphans = std::move(_children[entity.index]);
    _children[entity.index].clear();
    // Remove every component of the entity
    for (std::size_t column = 0; column < kMaxComponents; column++) {
        if (_rows[entity.index][column] != 0) {
            removeFromPool(entity, column);
        }
    }
    auto& version = _versions[entity.index];
    version++;
    if (version == 0) {
        version = 1;
    }
    _free.push(entity.index);
    for (const auto child : orphans) {
        if (alive(child)) {
            destroy(child);
        }
    }
}

void World::setParent(Entity child, Entity parent) {
    requireAlive(child, "setParent");
    if (parent != Entity{}) {
        requireAlive(parent, "setParent");
    }
    for (auto ancestor = parent; ancestor != Entity{};
         ancestor = _parents[ancestor.index]) {
        if (ancestor == child) {
            throw HierarchyCycle(describe(child) + " in setParent");
        }
    }
    auto& current = _parents[child.index];
    if (current == parent) {
        return;
    }
    if (alive(current)) {
        std::erase(_children[current.index], child);
    }
    current = parent;
    if (parent != Entity{}) {
        _children[parent.index].push_back(child);
    }
}

Entity World::parent(Entity entity) const {
    requireAlive(entity, "parent");
    return _parents[entity.index];
}

const std::vector<Entity>& World::children(Entity entity) const {
    requireAlive(entity, "children");
    return _children[entity.index];
}

void World::removeFromPool(Entity entity, std::size_t column) {
    const auto position = _rows[entity.index][column] - 1;
    const Entity moved = _pools[column]->removeAt(position);
    if (moved != Entity{}) {
        _rows[moved.index][column] = position + 1;
    }
    _rows[entity.index][column] = 0;
}

void World::requireAlive(Entity entity, const char* where) const {
    if (!alive(entity)) {
        throw InvalidEntity(describe(entity) + " in " + where);
    }
}

void World::requireComponent(Entity entity, std::size_t column,
                             const char* where) const {
    requireAlive(entity, where);
    if (_rows[entity.index][column] == 0) {
        throw ComponentNotFound(describe(entity) + " in " + where +
                                ", component #" + std::to_string(column));
    }
}

void World::requireNoComponent(Entity entity, std::size_t column,
                               const char* where) const {
    requireAlive(entity, where);
    if (_rows[entity.index][column] != 0) {
        throw ComponentAlreadyExists(describe(entity) + " in " + where +
                                     ", component #" + std::to_string(column));
    }
}

bool World::resourceExists(std::size_t index) const {
    return index < _resources.size() && _resources[index] != nullptr;
}

void World::requireResource(std::size_t index, const char* where) const {
    if (!resourceExists(index)) {
        throw ResourceNotFound("#" + std::to_string(index) + " in " + where);
    }
}

void World::requireNoResource(std::size_t index, const char* where) const {
    if (resourceExists(index)) {
        throw ResourceAlreadyExists("#" + std::to_string(index) + " in " +
                                    where);
    }
}

bool World::alive(Entity entity) const {
    return entity.index < _versions.size() &&
           _versions[entity.index] == entity.version;
}

}  // namespace ampersand::core::ecs
