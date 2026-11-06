#pragma once

#include <ampersand/core/ecs/ISystem.hpp>

namespace ampersand::render {

class IRenderer;
struct Renderer;

/**
 * @brief Draws one whole frame with the Renderer resource: clears the window,
 * sets the camera, draws every Sprite, Text, Rectangle, Circle and Line at its
 * world transform (see core::worldTransform, sorted by layer), then displays
 * it.
 */
class RenderSystem final : public core::ecs::ISystem {
public:
    /**
     * @brief Syncs the Renderer resource with its window, then, if the window
     * is open, clears with the ClearColor resource (black without it), uses
     * the Camera resource (default view without it), draws every entity with
     * a Transform and a Sprite, Text, Rectangle, Circle or Line, and ends the
     * frame.
     *
     * Entities without a Transform are skipped.
     *
     * @param world World holding the entities and the Renderer resource (and
     * the ClearColor and Camera resources, if any).
     * @throws ResourceNotFound if the World has no Renderer.
     * @throws std::invalid_argument if the Camera is not finite or its size
     * is not positive.
     */
    void update(core::ecs::World& world) override;

private:
    /**
     * @brief Closes the window if the game set `isOpen` to false, then copies
     * the real `isOpen` and `size` of the window into the resource.
     *
     * @return true if the window is open (there is something to draw).
     */
    static bool syncWindow(Renderer& window);

    /**
     * @brief Draws the component T of every entity that has a Transform.
     *
     * @tparam T Drawable component (Sprite, Text, Rectangle, Circle, Line).
     */
    template <typename T>
    static void drawAll(core::ecs::World& world, IRenderer& renderer);
};

}  // namespace ampersand::render
