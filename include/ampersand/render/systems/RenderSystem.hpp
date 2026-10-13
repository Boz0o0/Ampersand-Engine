#pragma once

#include <ampersand/core/ecs/ISystem.hpp>
#include <ampersand/render/IRenderer.hpp>

namespace ampersand::render {

/**
 * @brief Draws one whole frame: clears the window, sets the camera, draws
 * every Sprite and Text at its Transform (sorted by z), then displays it.
 */
class RenderSystem final : public core::ecs::ISystem {
public:
    /**
     * @brief Creates the system.
     *
     * @param renderer Renderer to draw with. Must outlive the system.
     */
    explicit RenderSystem(IRenderer& renderer);

    /**
     * @brief Clears with the ClearColor resource (black without it), uses the
     * Camera resource (default view without it), draws every entity with a
     * Transform and a Sprite or a Text, then ends the frame.
     *
     * Entities without a Transform are skipped.
     *
     * @param world World holding the entities (and the ClearColor and Camera
     * resources, if any).
     * @throws std::invalid_argument if the Camera is not finite or its size
     * is not positive, or if a Transform z is not finite.
     */
    void update(core::ecs::World& world) override;

private:
    IRenderer& _renderer;
};

}  // namespace ampersand::render
