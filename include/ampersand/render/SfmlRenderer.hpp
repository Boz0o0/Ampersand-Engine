#pragma once

#include <SFML/Graphics.hpp>
#include <ampersand/render/IRenderer.hpp>
#include <unordered_map>

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
  [[nodiscard]] bool isOpen() const override { return window_.isOpen(); }

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
   * @brief Loads a texture from disk under a generated texture identifier.
   *
   * @param path File system path to the texture image.
   * @return Identifier assigned to the loaded texture, or 0 on failure.
   */
  [[nodiscard]] TextureId loadTexture(
      const std::filesystem::path& path) override;

  /**
   * @brief Begins a new frame and clears the render target.
   *
   * @param clear Color to use when clearing the framebuffer.
   */
  void beginFrame(Color clear) override;

  /**
   * @brief Draws a sprite at the provided position in the current frame.
   *
   * @param sprite Sprite to render.
   * @param position Screen-space origin where the sprite is drawn.
   */
  void draw(const Sprite& sprite, core::math::Vec2f position) override;

  /**
   * @brief Displays the rendered content on the active window.
   */
  void endFrame() override;

 private:
  sf::RenderWindow window_;
  std::unordered_map<TextureId, sf::Texture> textures_;
  TextureId nextId_{1};
};

}  // namespace ampersand::render
