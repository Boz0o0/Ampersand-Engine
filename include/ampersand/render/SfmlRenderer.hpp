#pragma once

#include <SFML/Graphics.hpp>
#include <ampersand/render/IRenderer.hpp>
#include <cstdint>
#include <functional>
#include <unordered_map>
#include <vector>

namespace ampersand::render {

/**
 * @brief SFML-backed implementation of the renderer interface.
 *
 * This implementation translates the engine abstraction to SFML window and
 * drawing primitives, including input events and texture loading.
 */
class SfmlRenderer final : public IRenderer {
public:
    /**
     * @brief Opens a window using the provided configuration.
     *
     * @param config Initial window settings.
     * @return true if the underlying SFML window was created successfully.
     */
    bool open(const WindowConfig& config) override;

    /**
     * @brief Closes the current SFML window.
     */
    void close() override;

    /**
     * @brief Checks whether the window remains open.
     *
     * @return true if the SFML window is currently open.
     */
    [[nodiscard]] bool isOpen() const override { return _window.isOpen(); }

    /**
     * @brief Retrieves the size of the SFML window.
     *
     * @return Current pixel dimensions of the window.
     */
    [[nodiscard]] core::math::Vec2u size() const override;

    /**
     * @brief Polls the next queued event from the SFML event loop.
     *
     * @return The next event if one exists, otherwise std::nullopt.
     */
    std::optional<Event> pollEvent() override;

    /**
     * @brief Sets the world-space view used by the next draws, until
     * resetView() is called.
     *
     * @param camera Center and size of the visible area, in world units.
     * @throws std::invalid_argument if the camera is not finite or its size
     * is not positive.
     */
    void setView(const Camera& camera) override;

    /**
     * @brief Restores the default view, which covers the window in pixels and
     * follows its resizes.
     */
    void resetView() override;

    /**
     * @brief Loads a texture from disk under a generated texture identifier.
     *
     * @param path File system path to the texture image.
     * @return Identifier assigned to the loaded texture, or 0 on failure.
     */
    [[nodiscard]] TextureId loadTexture(
        const std::filesystem::path& path) override;

    /**
     * @brief Loads a font from disk under a generated font identifier.
     *
     * @param path File system path to the font file.
     * @return Identifier assigned to the loaded font, or 0 on failure.
     */
    [[nodiscard]] FontId loadFont(const std::filesystem::path& path) override;

    /**
     * @brief Begins a new frame and clears the render target.
     *
     * @param clear Color to use when clearing the framebuffer.
     */
    void beginFrame(Color clear) override;

    /**
     * @brief Queues a sprite, drawn at endFrame() in layer order.
     *
     * An unknown texture draws nothing; an empty source rect uses the whole
     * texture.
     *
     * @param sprite Texture, source rect, tint and layer of the sprite.
     * @param transform World transform of the sprite.
     */
    void draw(const Sprite& sprite, const core::Transform& transform) override;

    /**
     * @brief Queues a filled or outlined rectangle, drawn at endFrame().
     *
     * @param rectangle Size, colors and layer of the rectangle.
     * @param transform World transform of the rectangle.
     */
    void draw(const Rectangle& rectangle,
              const core::Transform& transform) override;

    /**
     * @brief Queues a filled or outlined circle, drawn at endFrame().
     *
     * @param circle Radius, colors and layer of the circle.
     * @param transform World transform of the circle.
     */
    void draw(const Circle& circle, const core::Transform& transform) override;

    /**
     * @brief Queues a one-pixel line segment, drawn at endFrame().
     *
     * @param line Endpoints, color and layer of the line.
     * @param transform World transform applied to both endpoints.
     */
    void draw(const Line& line, const core::Transform& transform) override;

    /**
     * @brief Queues a text, drawn at endFrame(). An unknown font draws
     * nothing.
     *
     * @param text String, font, size, color and layer of the text.
     * @param transform World transform of the text.
     */
    void draw(const Text& text, const core::Transform& transform) override;

    /**
     * @brief Draws the queued commands sorted by layer, then displays the
     * window. The sprites that follow each other with the same texture are
     * drawn in one draw call.
     */
    void endFrame() override;

private:
    /** @brief A queued draw, run by endFrame() in layer order. */
    struct DrawCommand {
        std::int32_t layer{0};
        std::function<void()> draw;
    };

    /**
     * @brief Places an SFML drawable with an engine transform.
     *
     * @param drawable Drawable to move, scale, turn and pivot.
     * @param transform Position, scale, rotation (around z) and origin.
     */
    static void applyTransform(sf::Transformable& drawable,
                               const core::Transform& transform);

    /**
     * @brief Rebuilds _defaultView to cover the window in pixels. Does
     * nothing while the window has no area.
     */
    void updateDefaultView();

    /**
     * @brief Adds the quad of a sprite to _batch, after flushing it if it
     * holds another texture.
     *
     * @param sprite Placed sprite, as sf::Sprite would draw it.
     * @param texture Identifier of the texture of the sprite.
     */
    void batchSprite(const sf::Sprite& sprite, TextureId texture);

    /** @brief Draws the sprites of _batch in one draw call, then empties it. */
    void flush();

    sf::RenderWindow _window;
    std::unordered_map<TextureId, sf::Texture> _textures;
    std::unordered_map<FontId, sf::Font> _fonts;
    std::vector<DrawCommand> _drawCommands;
    std::vector<sf::Vertex> _batch;  // Sprites of _batchTexture to draw
    TextureId _batchTexture{};
    sf::View _defaultView;
    TextureId _nextId{1};
    FontId _nextFontId{1};
    bool _customView{};
};

}  // namespace ampersand::render
