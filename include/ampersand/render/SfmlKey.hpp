#pragma once

#include <SFML/Window/Keyboard.hpp>
#include <ampersand/render/Event.hpp>

namespace ampersand::render {

inline Key toKey(sf::Keyboard::Key key) {
  switch (key) {
    case sf::Keyboard::Key::Up:
      return Key::Up;
    case sf::Keyboard::Key::Down:
      return Key::Down;
    case sf::Keyboard::Key::Left:
      return Key::Left;
    case sf::Keyboard::Key::Right:
      return Key::Right;
    case sf::Keyboard::Key::Space:
      return Key::Space;
    case sf::Keyboard::Key::Escape:
      return Key::Escape;
    default:
      return Key::Unknown;
  }
}

}  // namespace ampersand::render
