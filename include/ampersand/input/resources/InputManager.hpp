#pragma once

#include <ampersand/core/math/Vec2.hpp>
#include <ampersand/input/Event.hpp>
#include <map>
#include <optional>
#include <set>
#include <string>
#include <variant>
#include <vector>

namespace ampersand::input {

/** @brief Any input event. */
using Event =
    std::variant<KeyPressed, KeyReleased, MousePressed, MouseReleased,
                 MouseMoved, JoystickButtonPressed, JoystickButtonReleased,
                 JoystickMoved, JoystickDisconnected, FocusLost>;

/**
 * @brief A key, a mouse button or a controller button, used by the axes and
 * virtual buttons. std::monostate is nothing: never held.
 */
using Binding =
    std::variant<std::monostate, KeyCode, MouseButton, JoystickButton>;

/**
 * @brief Axis read by name, from bindings and from a controller axis.
 *
 * The bindings give 1 while the positive one is held, -1 while the negative
 * one is held, 0 when both or none are held. The controller axis gives its
 * position, or 0 inside the dead zone. The axis is the one of the two that is
 * the farthest from 0.
 */
struct Axis {
    Binding positive;
    Binding negative;
    /** @brief Controller axis read too, if any. */
    std::optional<JoystickAxis> joystick;
    /** @brief Positions of the controller axis closer to 0 than this are 0. */
    float dead{0.2F};
    /** @brief Whether the position of the controller axis is negated. */
    bool invertJoystick = false;
};

/**
 * @brief Input of the current frame: what is held, what was pressed and
 * released during the frame, and where the mouse is.
 *
 * Filled each frame by the InputSystem with beginFrame and apply. The systems
 * of the game read it with the get functions, and name the axes and buttons
 * they read with setAxis and setButton.
 */
class InputManager {
public:
    /**
     * @brief Starts a new frame: empties the pressed and released sets. The
     * held keys and buttons, the mouse and the controller axes are kept.
     */
    void beginFrame();

    /**
     * @brief Applies an event to the held, pressed and released sets, the
     * mouse and the controller axes.
     *
     * A press of a key or button already held is ignored (the keyboard
     * repeats a held key), and so are the keys and buttons that are None.
     */
    void apply(const Event& event);

    /** @brief Names an axis, replacing the one of the same name if any. */
    void setAxis(const std::string& name, const Axis& axis);

    /**
     * @brief Names a virtual button held when one of its bindings is,
     * replacing the one of the same name if any.
     */
    void setButton(const std::string& name, std::vector<Binding> bindings);

    /** @brief Whether the key is held. */
    [[nodiscard]] bool getKey(KeyCode key) const;

    /** @brief Whether the key was pressed during this frame. */
    [[nodiscard]] bool getKeyDown(KeyCode key) const;

    /** @brief Whether the key was released during this frame. */
    [[nodiscard]] bool getKeyUp(KeyCode key) const;

    /** @brief Whether the mouse button is held. */
    [[nodiscard]] bool getMouseButton(MouseButton button) const;

    /** @brief Whether the mouse button was pressed during this frame. */
    [[nodiscard]] bool getMouseButtonDown(MouseButton button) const;

    /** @brief Whether the mouse button was released during this frame. */
    [[nodiscard]] bool getMouseButtonUp(MouseButton button) const;

    /** @brief Whether the controller button is held on any controller. */
    [[nodiscard]] bool getJoystickButton(JoystickButton button) const;

    /** @brief Whether the controller button was pressed during this frame. */
    [[nodiscard]] bool getJoystickButtonDown(JoystickButton button) const;

    /** @brief Whether the controller button was released during this frame. */
    [[nodiscard]] bool getJoystickButtonUp(JoystickButton button) const;

    /**
     * @brief Position of the controller axis (-1 to 1) on the controller where
     * it is the farthest from 0, 0 if it never moved.
     */
    [[nodiscard]] float getJoystickAxis(JoystickAxis axis) const;

    /** @brief Whether the binding is held. */
    [[nodiscard]] bool getBinding(Binding binding) const;

    /** @brief Whether the binding was pressed during this frame. */
    [[nodiscard]] bool getBindingDown(Binding binding) const;

    /** @brief Whether the binding was released during this frame. */
    [[nodiscard]] bool getBindingUp(Binding binding) const;

    /**
     * @brief Value of the axis, from -1 to 1 (see Axis).
     * @throws std::out_of_range if no axis has this name.
     */
    [[nodiscard]] float getAxisRaw(const std::string& name) const;

    /**
     * @brief Whether one of the bindings of the virtual button is held.
     * @throws std::out_of_range if no button has this name.
     */
    [[nodiscard]] bool getButton(const std::string& name) const;

    /**
     * @brief Whether one of the bindings of the virtual button was pressed
     * during this frame.
     * @throws std::out_of_range if no button has this name.
     */
    [[nodiscard]] bool getButtonDown(const std::string& name) const;

    /**
     * @brief Whether one of the bindings of the virtual button was released
     * during this frame.
     * @throws std::out_of_range if no button has this name.
     */
    [[nodiscard]] bool getButtonUp(const std::string& name) const;

    /** @brief Keys currently held. */
    [[nodiscard]] const std::set<KeyCode>& keysDown() const {
        return _keysDown;
    }

    /** @brief Keys pressed during the current frame. */
    [[nodiscard]] const std::set<KeyCode>& keysPressed() const {
        return _keysPressed;
    }

    /** @brief Keys released during the current frame. */
    [[nodiscard]] const std::set<KeyCode>& keysReleased() const {
        return _keysReleased;
    }

    /** @brief Mouse buttons currently held. */
    [[nodiscard]] const std::set<MouseButton>& buttonsDown() const {
        return _buttonsDown;
    }

    /** @brief Mouse buttons pressed during the current frame. */
    [[nodiscard]] const std::set<MouseButton>& buttonsPressed() const {
        return _buttonsPressed;
    }

    /** @brief Mouse buttons released during the current frame. */
    [[nodiscard]] const std::set<MouseButton>& buttonsReleased() const {
        return _buttonsReleased;
    }

    /** @brief Last known position of the mouse in the window, in pixels. */
    [[nodiscard]] core::math::Vec2i mouse() const { return _mouse; }

    /** @brief Controller buttons currently held, by controller index. */
    [[nodiscard]] const std::map<unsigned int, std::set<JoystickButton>>&
    joystickButtonsDown() const {
        return _joystickButtonsDown;
    }

    /** @brief Controller buttons pressed during the current frame. */
    [[nodiscard]] const std::set<JoystickButton>& joystickButtonsPressed()
        const {
        return _joystickButtonsPressed;
    }

    /** @brief Controller buttons released during the current frame. */
    [[nodiscard]] const std::set<JoystickButton>& joystickButtonsReleased()
        const {
        return _joystickButtonsReleased;
    }

    /**
     * @brief Last position of each controller axis that moved, -1 to 1, by
     * controller index.
     */
    [[nodiscard]] const std::map<unsigned int, std::map<JoystickAxis, float>>&
    joystickAxes() const {
        return _joystickAxes;
    }

private:
    /** @brief Holds the key, and marks it pressed if it was not held yet. */
    void handle(const KeyPressed& pressed);

    /** @brief Releases the key and marks it released. */
    void handle(const KeyReleased& released);

    /**
     * @brief Moves the mouse, holds the button, and marks it pressed if it
     * was not held yet.
     */
    void handle(const MousePressed& pressed);

    /** @brief Moves the mouse, releases the button and marks it released. */
    void handle(const MouseReleased& released);

    /** @brief Moves the mouse. */
    void handle(const MouseMoved& moved);

    /**
     * @brief Holds the controller button, and marks it pressed if it was not
     * held yet.
     */
    void handle(const JoystickButtonPressed& pressed);

    /** @brief Releases the controller button and marks it released. */
    void handle(const JoystickButtonReleased& released);

    /** @brief Moves the controller axis. */
    void handle(const JoystickMoved& moved);

    /**
     * @brief Forgets the held buttons and the axes of the controller: its
     * axes are 0 again. Its buttons are not marked released.
     */
    void handle(const JoystickDisconnected& disconnected);

    /**
     * @brief Releases each held key and mouse button, marking them released.
     * The controllers are left as they are: they are still reported without
     * the focus.
     */
    void handle(const FocusLost& lost);

    std::set<KeyCode> _keysDown;
    std::set<KeyCode> _keysPressed;
    std::set<KeyCode> _keysReleased;
    std::set<MouseButton> _buttonsDown;
    std::set<MouseButton> _buttonsPressed;
    std::set<MouseButton> _buttonsReleased;
    core::math::Vec2i _mouse;
    std::map<unsigned int, std::set<JoystickButton>> _joystickButtonsDown;
    std::set<JoystickButton> _joystickButtonsPressed;
    std::set<JoystickButton> _joystickButtonsReleased;
    std::map<unsigned int, std::map<JoystickAxis, float>> _joystickAxes;
    std::map<std::string, Axis> _axes;
    std::map<std::string, std::vector<Binding>> _buttons;
};

}  // namespace ampersand::input
