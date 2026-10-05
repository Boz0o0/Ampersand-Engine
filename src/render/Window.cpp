#include <ampersand/render/SfmlKey.hpp>
#include <ampersand/render/Window.hpp>
#include <utility>

namespace ampersand::render {

void Window::create(ampersand::core::math::Vec2u size, std::string title) {
  this->title_ = std::move(title);
  this->window_.create(sf::VideoMode({size.x, size.y}), this->title_);
}

void Window::destroy() { this->window_.close(); }

bool Window::isOpen() const { return this->window_.isOpen(); }

ampersand::core::math::Vec2u Window::size() const {
  const auto size = this->window_.getSize();
  return {size.x, size.y};
}

const std::string& Window::title() const { return this->title_; }

std::optional<Event> Window::pollEvent() {
  while (const auto event = this->window_.pollEvent()) {
    if (event->is<sf::Event::Closed>()) {
      return Closed{};
    }
    if (const auto* const resized = event->getIf<sf::Event::Resized>()) {
      return Resized{{resized->size.x, resized->size.y}};
    }
    if (const auto* const keyPressed = event->getIf<sf::Event::KeyPressed>()) {
      return KeyPressed{toKey(keyPressed->code)};
    }
    if (const auto* const keyReleased =
            event->getIf<sf::Event::KeyReleased>()) {
      return KeyReleased{toKey(keyReleased->code)};
    }
  }
  return std::nullopt;
}

void Window::clear() { this->window_.clear(); }

void Window::display() { this->window_.display(); }

}  // namespace ampersand::render
