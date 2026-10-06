#pragma once

#include <ampersand/core/math/Vec2.hpp>
#include <ampersand/render/Color.hpp>
#include <ampersand/render/Event.hpp>
#include <ampersand/render/Sprite.hpp>
#include <ampersand/render/WindowConfig.hpp>
#include <filesystem>
#include <optional>

namespace ampersand::render {

/**
 * @brief Abstract interface for all renderer backends.
 *
 * This interface defines the minimum functionality required to create a
 * windowed rendering context, process events, load textures and draw sprites.
 * Concrete implementations are expected to provide the platform-specific
 * behavior while keeping the high-level API uniform.
 */
class IRenderer {
 public:
  /**
   * @brief Destroys the renderer instance.
   */
  virtual ~IRenderer() = default;

  IRenderer(const IRenderer&) = delete;
  IRenderer& operator=(const IRenderer&) = delete;

  /**
   * @brief Opens the renderer using the provided window configuration.
   *
   * @param config Settings used to initialize the rendering window.
   * @return true if the renderer was successfully opened, false otherwise.
   */
  virtual bool open(const WindowConfig& config) = 0;

  /**
   * @brief Closes the renderer and releases any active window resources.
   */
  virtual void close() = 0;

  /**
   * @brief Checks whether the renderer is currently open.
   *
   * @return true if the rendering context is active, false otherwise.
   */
  [[nodiscard]] virtual bool isOpen() const = 0;

  /**
   * @brief Retrieves the current size of the rendering window.
   *
   * @return Dimensions of the window in pixels.
   */
  [[nodiscard]] virtual core::math::Vec2u size() const = 0;

  /**
   * @brief Polls the next queued event from the window.
   *
   * @return The next event if one is available, otherwise std::nullopt.
   */
  virtual std::optional<Event> pollEvent() = 0;

  /**
   * @brief Loads a texture from the provided file path.
   *
   * @param path Location of the image file to load.
   * @return Identifier of the loaded texture, or an invalid value if loading
   * failed.
   */
  [[nodiscard]] virtual TextureId loadTexture(
      const std::filesystem::path& path) = 0;

  /**
   * @brief Begins a new frame and clears the screen with the specified color.
   *
   * @param clear Color used to clear the framebuffer before drawing.
   */
  virtual void beginFrame(Color clear) = 0;

  /**
   * @brief Draws a sprite at the given position in the current frame.
   *
   * @param sprite Sprite to render.
   * @param position Screen-space position where the sprite should be drawn.
   */
  virtual void draw(const Sprite& sprite, core::math::Vec2f position) = 0;

  /**
   * @brief Presents the current frame to the display.
   */
  virtual void endFrame() = 0;

 protected:
  /**
   * @brief Creates a default renderer instance.
   */
  IRenderer() = default;
};

}  // namespace ampersand::render
