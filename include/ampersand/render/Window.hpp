#pragma once

#include <SFML/Graphics/RenderWindow.hpp>
#include <ampersand/core/math/Vec2.hpp>
#include <ampersand/render/Event.hpp>
#include <optional>
#include <string>

namespace ampersand::render {

/**
 * @brief Configuration used to initialize a window instance.
 */
struct WindowConfig {
  ampersand::core::math::Vec2u size;
  std::string title;
  bool vsync{true};
};

/**
 * @brief Lightweight window wrapper used by the rendering system.
 */
class Window {
 public:
  /**
   * @brief Creates a new window object in its default state.
   */
  Window() = default;

  /**
   * @brief Destroys the window and releases any owned resources.
   */
  ~Window() = default;

  Window(const Window&) = delete;
  Window& operator=(const Window&) = delete;
  Window(Window&&) noexcept = default;
  Window& operator=(Window&&) noexcept = default;

  /**
   * @brief Creates an operating system window with the given size and title.
   *
   * @param size Desired window dimensions in pixels.
   * @param title Title displayed by the window manager.
   */
  void create(ampersand::core::math::Vec2u size, std::string title);

  /**
   * @brief Closes the window and releases its native handle.
   */
  void destroy();

  /**
   * @brief Checks whether the window is currently open.
   *
   * @return true when the window has been created and remains active.
   */
  [[nodiscard]] bool isOpen() const;

  /**
   * @brief Queries the current window size.
   *
   * @return Pixel dimensions of the active window.
   */
  [[nodiscard]] ampersand::core::math::Vec2u size() const;

  /**
   * @brief Retrieves the current title of the window.
   *
   * @return Reference to the title string.
   */
  [[nodiscard]] const std::string& title() const;

  /**
   * @brief Polls the next queued event from the window.
   *
   * @return The next available event, or std::nullopt when the queue is empty.
   */
  std::optional<Event> pollEvent();

  /**
   * @brief Clears the render target with the default color.
   */
  void clear();

  /**
   * @brief Presents the back buffer to the display.
   */
  void display();

 private:
  sf::RenderWindow window_;
  std::string title_;
};

}  // namespace ampersand::render
