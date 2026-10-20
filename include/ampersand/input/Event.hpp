#pragma once

#include <ampersand/core/math/Vec2.hpp>
#include <cstdint>

namespace ampersand::input {

/**
 * @brief Enumeration of supported keyboard keys.
 *
 * A key is a physical position on the keyboard, named after the QWERTY
 * layout: on AZERTY, the key labelled Z is KeyCode::W.
 */
enum class KeyCode : std::uint8_t {
    A,
    B,
    C,
    D,
    E,
    F,
    G,
    H,
    I,
    J,
    K,
    L,
    M,
    N,
    O,
    P,
    Q,
    R,
    S,
    T,
    U,
    V,
    W,
    X,
    Y,
    Z,
    Alpha0,
    Alpha1,
    Alpha2,
    Alpha3,
    Alpha4,
    Alpha5,
    Alpha6,
    Alpha7,
    Alpha8,
    Alpha9,
    UpArrow,
    DownArrow,
    LeftArrow,
    RightArrow,
    Space,
    Return,
    Escape,
    Tab,
    Backspace,
    LeftShift,
    RightShift,
    LeftControl,
    RightControl,
    LeftAlt,
    RightAlt,
    None,
};

/**
 * @brief Enumeration of supported mouse buttons.
 */
enum class MouseButton : std::uint8_t { Left, Right, Middle, None };

/**
 * @brief Enumeration of supported controller buttons, by index: the button
 * behind an index depends on the controller.
 */
enum class JoystickButton : std::uint8_t {
    Button0,
    Button1,
    Button2,
    Button3,
    Button4,
    Button5,
    Button6,
    Button7,
    Button8,
    Button9,
    Button10,
    Button11,
    Button12,
    Button13,
    Button14,
    Button15,
    Button16,
    Button17,
    Button18,
    Button19,
    Button20,
    Button21,
    Button22,
    Button23,
    Button24,
    Button25,
    Button26,
    Button27,
    Button28,
    Button29,
    Button30,
    Button31,
    None,
};

/**
 * @brief Enumeration of supported controller axes (sticks, triggers, d-pad).
 */
enum class JoystickAxis : std::uint8_t { X, Y, Z, R, U, V, PovX, PovY };

/**
 * @brief Event triggered when a key is pressed.
 */
struct KeyPressed {
    KeyCode key;
};

/**
 * @brief Event triggered when a key is released.
 */
struct KeyReleased {
    KeyCode key;
};

/**
 * @brief Event triggered when a mouse button is pressed.
 */
struct MousePressed {
    MouseButton button;
    /** @brief Position of the mouse in the window, in pixels. */
    core::math::Vec2i position;
};

/**
 * @brief Event triggered when a mouse button is released.
 */
struct MouseReleased {
    MouseButton button;
    /** @brief Position of the mouse in the window, in pixels. */
    core::math::Vec2i position;
};

/**
 * @brief Event triggered when the mouse moves inside the window.
 */
struct MouseMoved {
    /** @brief Position of the mouse in the window, in pixels. */
    core::math::Vec2i position;
};

/**
 * @brief Event triggered when a button of any controller is pressed.
 */
struct JoystickButtonPressed {
    /** @brief Index of the controller. */
    unsigned int joystick;
    JoystickButton button;
};

/**
 * @brief Event triggered when a button of any controller is released.
 */
struct JoystickButtonReleased {
    /** @brief Index of the controller. */
    unsigned int joystick;
    JoystickButton button;
};

/**
 * @brief Event triggered when an axis of any controller moves.
 */
struct JoystickMoved {
    /** @brief Index of the controller. */
    unsigned int joystick;
    JoystickAxis axis;
    /** @brief New position of the axis, from -1 to 1. */
    float position;
};

/**
 * @brief Event triggered when a controller is disconnected.
 */
struct JoystickDisconnected {
    /** @brief Index of the controller. */
    unsigned int joystick;
};

/**
 * @brief Event triggered when the window loses the focus: its keys and mouse
 * buttons are no longer reported until it gets it back. The controllers still
 * are.
 */
struct FocusLost {};

}  // namespace ampersand::input
