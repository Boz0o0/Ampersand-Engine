#include <ampersand/core/components/Transform.hpp>
#include <ampersand/core/ecs/World.hpp>
#include <ampersand/render/components/Sprite.hpp>
#include <ampersand/render/components/Text.hpp>
#include <ampersand/render/resources/Camera.hpp>
#include <ampersand/render/resources/ClearColor.hpp>
#include <ampersand/render/systems/RenderSystem.hpp>

namespace ampersand::render {

RenderSystem::RenderSystem(IRenderer& renderer) : _renderer(renderer) {}

void RenderSystem::update(core::ecs::World& world) {
    _renderer.beginFrame(world.hasResource<ClearColor>()
                             ? world.getResource<ClearColor>().value
                             : Color{});
    if (world.hasResource<Camera>()) {
        _renderer.setView(world.getResource<Camera>());
    } else {
        _renderer.resetView();
    }
    for (const core::ecs::Entity entity : world.owners<Sprite>()) {
        if (!world.has<core::Transform>(entity)) {
            continue;
        }
        _renderer.draw(world.get<Sprite>(entity),
                       world.get<core::Transform>(entity));
    }
    for (const core::ecs::Entity entity : world.owners<Text>()) {
        if (!world.has<core::Transform>(entity)) {
            continue;
        }
        _renderer.draw(world.get<Text>(entity),
                       world.get<core::Transform>(entity));
    }
    _renderer.endFrame();
}

}  // namespace ampersand::render
