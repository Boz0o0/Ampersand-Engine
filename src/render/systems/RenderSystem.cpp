#include <ampersand/core/ecs/World.hpp>
#include <ampersand/core/hierarchy/WorldTransform.hpp>
#include <ampersand/render/Primitive.hpp>
#include <ampersand/render/components/Sprite.hpp>
#include <ampersand/render/components/Text.hpp>
#include <ampersand/render/resources/Camera.hpp>
#include <ampersand/render/resources/ClearColor.hpp>
#include <ampersand/render/resources/Renderer.hpp>
#include <ampersand/render/systems/RenderSystem.hpp>

namespace ampersand::render {

void RenderSystem::update(core::ecs::World& world) {
    auto& window = world.getResource<Renderer>();
    if (!syncWindow(window)) {
        return;
    }
    IRenderer& renderer = window.renderer;
    renderer.beginFrame(world.hasResource<ClearColor>()
                            ? world.getResource<ClearColor>().value
                            : Color{});
    if (world.hasResource<Camera>()) {
        renderer.setView(world.getResource<Camera>());
    } else {
        renderer.resetView();
    }
    drawAll<Sprite>(world, renderer);
    drawAll<Text>(world, renderer);
    drawAll<Rectangle>(world, renderer);
    drawAll<Circle>(world, renderer);
    drawAll<Line>(world, renderer);
    renderer.endFrame();
}

template <typename T>
void RenderSystem::drawAll(core::ecs::World& world, IRenderer& renderer) {
    for (const core::ecs::Entity entity : world.owners<T>()) {
        if (world.has<core::Transform>(entity)) {
            renderer.draw(world.get<T>(entity),
                          core::worldTransform(world, entity));
        }
    }
}

bool RenderSystem::syncWindow(Renderer& window) {
    if (!window.isOpen) {
        window.renderer.close();
    }
    window.isOpen = window.renderer.isOpen();
    window.size = window.renderer.size();
    return window.isOpen;
}

}  // namespace ampersand::render
