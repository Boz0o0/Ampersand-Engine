#include <algorithm>
#include <ampersand/input/resources/InputManager.hpp>
#include <cmath>
#include <string>
#include <utility>
#include <variant>
#include <vector>

namespace ampersand::input {

void InputManager::beginFrame() {
    _keysPressed.clear();
    _keysReleased.clear();
    _buttonsPressed.clear();
    _buttonsReleased.clear();
    _joystickButtonsPressed.clear();
    _joystickButtonsReleased.clear();
}

void InputManager::apply(const Event& event) {
    std::visit([this](const auto& value) { handle(value); }, event);
}

void InputManager::setAxis(const std::string& name, const Axis& axis) {
    _axes[name] = axis;
}

void InputManager::setButton(const std::string& name,
                             std::vector<Binding> bindings) {
    _buttons[name] = std::move(bindings);
}

bool InputManager::getKey(KeyCode key) const { return _keysDown.contains(key); }

bool InputManager::getKeyDown(KeyCode key) const {
    return _keysPressed.contains(key);
}

bool InputManager::getKeyUp(KeyCode key) const {
    return _keysReleased.contains(key);
}

bool InputManager::getMouseButton(MouseButton button) const {
    return _buttonsDown.contains(button);
}

bool InputManager::getMouseButtonDown(MouseButton button) const {
    return _buttonsPressed.contains(button);
}

bool InputManager::getMouseButtonUp(MouseButton button) const {
    return _buttonsReleased.contains(button);
}

bool InputManager::getJoystickButton(JoystickButton button) const {
    return std::ranges::any_of(
        _joystickButtonsDown,
        [button](const auto& held) { return held.second.contains(button); });
}

bool InputManager::getJoystickButtonDown(JoystickButton button) const {
    return _joystickButtonsPressed.contains(button);
}

bool InputManager::getJoystickButtonUp(JoystickButton button) const {
    return _joystickButtonsReleased.contains(button);
}

float InputManager::getJoystickAxis(JoystickAxis axis) const {
    float farthest = 0.0F;
    for (const auto& [joystick, positions] : _joystickAxes) {
        const auto position = positions.find(axis);
        if (position != positions.end() &&
            std::abs(position->second) > std::abs(farthest)) {
            farthest = position->second;
        }
    }
    return farthest;
}

bool InputManager::getBinding(Binding binding) const {
    if (std::holds_alternative<KeyCode>(binding)) {
        return getKey(std::get<KeyCode>(binding));
    }
    if (std::holds_alternative<MouseButton>(binding)) {
        return getMouseButton(std::get<MouseButton>(binding));
    }
    if (std::holds_alternative<JoystickButton>(binding)) {
        return getJoystickButton(std::get<JoystickButton>(binding));
    }
    return false;
}

bool InputManager::getBindingDown(Binding binding) const {
    if (std::holds_alternative<KeyCode>(binding)) {
        return getKeyDown(std::get<KeyCode>(binding));
    }
    if (std::holds_alternative<MouseButton>(binding)) {
        return getMouseButtonDown(std::get<MouseButton>(binding));
    }
    if (std::holds_alternative<JoystickButton>(binding)) {
        return getJoystickButtonDown(std::get<JoystickButton>(binding));
    }
    return false;
}

bool InputManager::getBindingUp(Binding binding) const {
    if (std::holds_alternative<KeyCode>(binding)) {
        return getKeyUp(std::get<KeyCode>(binding));
    }
    if (std::holds_alternative<MouseButton>(binding)) {
        return getMouseButtonUp(std::get<MouseButton>(binding));
    }
    if (std::holds_alternative<JoystickButton>(binding)) {
        return getJoystickButtonUp(std::get<JoystickButton>(binding));
    }
    return false;
}

float InputManager::getAxisRaw(const std::string& name) const {
    const Axis& axis = _axes.at(name);
    const float bindings = static_cast<float>(getBinding(axis.positive)) -
                           static_cast<float>(getBinding(axis.negative));
    if (!axis.joystick) {
        return bindings;
    }
    float joystick = getJoystickAxis(*axis.joystick);
    if (std::abs(joystick) < axis.dead) {
        joystick = 0.0F;
    }
    if (axis.invertJoystick) {
        joystick = -joystick;
    }
    return std::abs(joystick) > std::abs(bindings) ? joystick : bindings;
}

bool InputManager::getButton(const std::string& name) const {
    return std::ranges::any_of(_buttons.at(name), [this](Binding binding) {
        return getBinding(binding);
    });
}

bool InputManager::getButtonDown(const std::string& name) const {
    return std::ranges::any_of(_buttons.at(name), [this](Binding binding) {
        return getBindingDown(binding);
    });
}

bool InputManager::getButtonUp(const std::string& name) const {
    return std::ranges::any_of(_buttons.at(name), [this](Binding binding) {
        return getBindingUp(binding);
    });
}

void InputManager::handle(const KeyPressed& pressed) {
    if (pressed.key != KeyCode::None && _keysDown.insert(pressed.key).second) {
        _keysPressed.insert(pressed.key);
    }
}

void InputManager::handle(const KeyReleased& released) {
    if (released.key == KeyCode::None) {
        return;
    }
    _keysDown.erase(released.key);
    _keysReleased.insert(released.key);
}

void InputManager::handle(const MousePressed& pressed) {
    _mouse = pressed.position;
    if (pressed.button != MouseButton::None &&
        _buttonsDown.insert(pressed.button).second) {
        _buttonsPressed.insert(pressed.button);
    }
}

void InputManager::handle(const MouseReleased& released) {
    _mouse = released.position;
    if (released.button == MouseButton::None) {
        return;
    }
    _buttonsDown.erase(released.button);
    _buttonsReleased.insert(released.button);
}

void InputManager::handle(const MouseMoved& moved) { _mouse = moved.position; }

void InputManager::handle(const JoystickButtonPressed& pressed) {
    if (pressed.button != JoystickButton::None &&
        _joystickButtonsDown[pressed.joystick].insert(pressed.button).second) {
        _joystickButtonsPressed.insert(pressed.button);
    }
}

void InputManager::handle(const JoystickButtonReleased& released) {
    if (released.button == JoystickButton::None) {
        return;
    }
    _joystickButtonsDown[released.joystick].erase(released.button);
    _joystickButtonsReleased.insert(released.button);
}

void InputManager::handle(const JoystickMoved& moved) {
    _joystickAxes[moved.joystick][moved.axis] = moved.position;
}

void InputManager::handle(const FocusLost& /*lost*/) {
    _keysReleased.insert(_keysDown.begin(), _keysDown.end());
    _buttonsReleased.insert(_buttonsDown.begin(), _buttonsDown.end());
    _keysDown.clear();
    _buttonsDown.clear();
}

}  // namespace ampersand::input
