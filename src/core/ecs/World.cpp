#include <ampersand/core/ecs/ECSExceptions.hpp>
#include <ampersand/core/ecs/World.hpp>
#include <string>

namespace ampersand::core::ecs {

World::World(std::uint32_t maxEntities) : _maxEntities(maxEntities) {
    _versions.reserve(maxEntities);
}

Entity World::create() {
    if (_versions.size() >= _maxEntities) {
        throw EntityLimitReached(std::to_string(_maxEntities) + " entities");
    }
    const auto index = static_cast<std::uint32_t>(_versions.size());
    _versions.push_back(1);
    return {index, _versions[index]};
}

bool World::alive(Entity entity) const {
    return entity.index < _versions.size() &&
           _versions[entity.index] == entity.version;
}

}  // namespace ampersand::core::ecs
