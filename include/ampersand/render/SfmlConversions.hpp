#pragma once

#include <SFML/Graphics/Color.hpp>
#include <SFML/Window/Event.hpp>
#include <SFML/Window/Joystick.hpp>
#include <SFML/Window/Keyboard.hpp>
#include <SFML/Window/Mouse.hpp>
#include <ampersand/input/Event.hpp>
#include <ampersand/render/Color.hpp>
#include <ampersand/render/Event.hpp>
#include <array>
#include <optional>
#include <utility>

namespace ampersand::render {

inline sf::Color toSfColor(Color color) {
    return {color.r, color.g, color.b, color.a};
}

/**
 * @brief SFML scancodes reported to the engine, by physical position on the
 * keyboard: any other key is KeyCode::None.
 */
inline constexpr auto kKeys =
    std::to_array<std::pair<sf::Keyboard::Scancode, input::KeyCode>>({
        {sf::Keyboard::Scan::A, input::KeyCode::A},
        {sf::Keyboard::Scan::B, input::KeyCode::B},
        {sf::Keyboard::Scan::C, input::KeyCode::C},
        {sf::Keyboard::Scan::D, input::KeyCode::D},
        {sf::Keyboard::Scan::E, input::KeyCode::E},
        {sf::Keyboard::Scan::F, input::KeyCode::F},
        {sf::Keyboard::Scan::G, input::KeyCode::G},
        {sf::Keyboard::Scan::H, input::KeyCode::H},
        {sf::Keyboard::Scan::I, input::KeyCode::I},
        {sf::Keyboard::Scan::J, input::KeyCode::J},
        {sf::Keyboard::Scan::K, input::KeyCode::K},
        {sf::Keyboard::Scan::L, input::KeyCode::L},
        {sf::Keyboard::Scan::M, input::KeyCode::M},
        {sf::Keyboard::Scan::N, input::KeyCode::N},
        {sf::Keyboard::Scan::O, input::KeyCode::O},
        {sf::Keyboard::Scan::P, input::KeyCode::P},
        {sf::Keyboard::Scan::Q, input::KeyCode::Q},
        {sf::Keyboard::Scan::R, input::KeyCode::R},
        {sf::Keyboard::Scan::S, input::KeyCode::S},
        {sf::Keyboard::Scan::T, input::KeyCode::T},
        {sf::Keyboard::Scan::U, input::KeyCode::U},
        {sf::Keyboard::Scan::V, input::KeyCode::V},
        {sf::Keyboard::Scan::W, input::KeyCode::W},
        {sf::Keyboard::Scan::X, input::KeyCode::X},
        {sf::Keyboard::Scan::Y, input::KeyCode::Y},
        {sf::Keyboard::Scan::Z, input::KeyCode::Z},
        {sf::Keyboard::Scan::Num0, input::KeyCode::Alpha0},
        {sf::Keyboard::Scan::Num1, input::KeyCode::Alpha1},
        {sf::Keyboard::Scan::Num2, input::KeyCode::Alpha2},
        {sf::Keyboard::Scan::Num3, input::KeyCode::Alpha3},
        {sf::Keyboard::Scan::Num4, input::KeyCode::Alpha4},
        {sf::Keyboard::Scan::Num5, input::KeyCode::Alpha5},
        {sf::Keyboard::Scan::Num6, input::KeyCode::Alpha6},
        {sf::Keyboard::Scan::Num7, input::KeyCode::Alpha7},
        {sf::Keyboard::Scan::Num8, input::KeyCode::Alpha8},
        {sf::Keyboard::Scan::Num9, input::KeyCode::Alpha9},
        {sf::Keyboard::Scan::Up, input::KeyCode::UpArrow},
        {sf::Keyboard::Scan::Down, input::KeyCode::DownArrow},
        {sf::Keyboard::Scan::Left, input::KeyCode::LeftArrow},
        {sf::Keyboard::Scan::Right, input::KeyCode::RightArrow},
        {sf::Keyboard::Scan::Space, input::KeyCode::Space},
        {sf::Keyboard::Scan::Enter, input::KeyCode::Return},
        {sf::Keyboard::Scan::Escape, input::KeyCode::Escape},
        {sf::Keyboard::Scan::Tab, input::KeyCode::Tab},
        {sf::Keyboard::Scan::Backspace, input::KeyCode::Backspace},
        {sf::Keyboard::Scan::LShift, input::KeyCode::LeftShift},
        {sf::Keyboard::Scan::RShift, input::KeyCode::RightShift},
        {sf::Keyboard::Scan::LControl, input::KeyCode::LeftControl},
        {sf::Keyboard::Scan::RControl, input::KeyCode::RightControl},
        {sf::Keyboard::Scan::LAlt, input::KeyCode::LeftAlt},
        {sf::Keyboard::Scan::RAlt, input::KeyCode::RightAlt},
    });

inline input::KeyCode toKey(sf::Keyboard::Scancode key) {
    for (const auto& [sfmlKey, engineKey] : kKeys) {
        if (sfmlKey == key) {
            return engineKey;
        }
    }
    return input::KeyCode::None;
}

inline input::MouseButton toMouseButton(sf::Mouse::Button button) {
    switch (button) {
        case sf::Mouse::Button::Left:
            return input::MouseButton::Left;
        case sf::Mouse::Button::Right:
            return input::MouseButton::Right;
        case sf::Mouse::Button::Middle:
            return input::MouseButton::Middle;
        default:
            return input::MouseButton::None;
    }
}

inline input::JoystickButton toJoystickButton(unsigned int button) {
    if (button >= static_cast<unsigned int>(input::JoystickButton::None)) {
        return input::JoystickButton::None;
    }
    return static_cast<input::JoystickButton>(button);
}

inline std::optional<input::JoystickAxis> toJoystickAxis(
    sf::Joystick::Axis axis) {
    switch (axis) {
        case sf::Joystick::Axis::X:
            return input::JoystickAxis::X;
        case sf::Joystick::Axis::Y:
            return input::JoystickAxis::Y;
        case sf::Joystick::Axis::Z:
            return input::JoystickAxis::Z;
        case sf::Joystick::Axis::R:
            return input::JoystickAxis::R;
        case sf::Joystick::Axis::U:
            return input::JoystickAxis::U;
        case sf::Joystick::Axis::V:
            return input::JoystickAxis::V;
        case sf::Joystick::Axis::PovX:
            return input::JoystickAxis::PovX;
        case sf::Joystick::Axis::PovY:
            return input::JoystickAxis::PovY;
        default:
            return std::nullopt;
    }
}

inline std::optional<Event> toEvent(const sf::Event::Closed& /*closed*/) {
    return Closed{};
}

inline std::optional<Event> toEvent(const sf::Event::Resized& resized) {
    return Resized{{resized.size.x, resized.size.y}};
}

inline std::optional<Event> toEvent(const sf::Event::KeyPressed& pressed) {
    return input::KeyPressed{toKey(pressed.scancode)};
}

inline std::optional<Event> toEvent(const sf::Event::KeyReleased& released) {
    return input::KeyReleased{toKey(released.scancode)};
}

inline std::optional<Event> toEvent(
    const sf::Event::MouseButtonPressed& pressed) {
    return input::MousePressed{
        .button = toMouseButton(pressed.button),
        .position = {pressed.position.x, pressed.position.y},
    };
}

inline std::optional<Event> toEvent(
    const sf::Event::MouseButtonReleased& released) {
    return input::MouseReleased{
        .button = toMouseButton(released.button),
        .position = {released.position.x, released.position.y},
    };
}

inline std::optional<Event> toEvent(const sf::Event::MouseMoved& moved) {
    return input::MouseMoved{{moved.position.x, moved.position.y}};
}

inline std::optional<Event> toEvent(
    const sf::Event::JoystickButtonPressed& pressed) {
    return input::JoystickButtonPressed{
        .joystick = pressed.joystickId,
        .button = toJoystickButton(pressed.button),
    };
}

inline std::optional<Event> toEvent(
    const sf::Event::JoystickButtonReleased& released) {
    return input::JoystickButtonReleased{
        .joystick = released.joystickId,
        .button = toJoystickButton(released.button),
    };
}

/**
 * @brief SFML gives the position from -100 to 100, the engine from -1 to 1.
 * An unknown axis is not reported.
 */
inline std::optional<Event> toEvent(const sf::Event::JoystickMoved& moved) {
    const std::optional<input::JoystickAxis> axis = toJoystickAxis(moved.axis);
    if (!axis) {
        return std::nullopt;
    }
    return input::JoystickMoved{.joystick = moved.joystickId,
                                .axis = *axis,
                                .position = moved.position / 100.0F};
}

inline std::optional<Event> toEvent(
    const sf::Event::JoystickDisconnected& disconnected) {
    return input::JoystickDisconnected{disconnected.joystickId};
}

inline std::optional<Event> toEvent(const sf::Event::FocusLost& /*lost*/) {
    return input::FocusLost{};
}

/** @brief Any other SFML event is not reported to the engine. */
template <typename SfmlEvent>
std::optional<Event> toEvent(const SfmlEvent& /*other*/) {
    return std::nullopt;
}

}  // namespace ampersand::render
