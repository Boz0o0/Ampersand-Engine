#include <ampersand/core/ecs/ECSExceptions.hpp>
#include <ampersand/core/ecs/World.hpp>
#include <string>

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
}

Entity World::create() {
    if (!_free.empty()) {
        const auto index = _free.front();
        _free.pop();
        return {index, _versions[index]};
    }
    if (_versions.size() >= _maxEntities) {  // if no free slot and all rows
                                             // used
        throw EntityLimitReached(std::to_string(_maxEntities) + " entities");
    }
    const auto index = static_cast<std::uint32_t>(_versions.size());
    _versions.push_back(1);
    _rows.emplace_back();  // every cell is 0
    return {index, _versions[index]};
}

void World::destroy(Entity entity) {
    if (!alive(entity)) {
        throw InvalidEntity(describe(entity) + " in destroy");
    }
    auto& version = _versions[entity.index];
    version++;
    if (version == 0) {
        version = 1;
    }
    _free.push(entity.index);
}

bool World::alive(Entity entity) const {
    return entity.index < _versions.size() &&
           _versions[entity.index] == entity.version;
}

}  // namespace ampersand::core::ecs
