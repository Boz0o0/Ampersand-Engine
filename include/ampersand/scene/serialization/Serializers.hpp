#pragma once

#include <ampersand/core/components/Transform.hpp>
#include <ampersand/core/math/Rect.hpp>
#include <ampersand/core/math/Vec2.hpp>
#include <ampersand/core/math/Vec3.hpp>
#include <nlohmann/json.hpp>

#ifdef AMPERSAND_MODULE_RENDER
#include <ampersand/render/Color.hpp>
#include <ampersand/render/WindowConfig.hpp>
#include <ampersand/render/components/Animation.hpp>
#include <ampersand/render/components/AnimationStateMachine.hpp>
#include <ampersand/render/components/Primitive.hpp>
#include <ampersand/render/resources/Camera.hpp>
#include <ampersand/render/resources/ClearColor.hpp>
#endif

#ifdef AMPERSAND_MODULE_PHYSICS
#include <ampersand/physics/components/BoxCollider.hpp>
#include <ampersand/physics/components/Gravity.hpp>
#include <ampersand/physics/components/Velocity.hpp>
#include <ampersand/physics/resources/GravityForce.hpp>
#endif

#ifdef AMPERSAND_MODULE_INPUT
#include <ampersand/input/resources/InputManager.hpp>
#endif

/**
 * @file Serializers.hpp
 * @brief JSON conversions of the engine types read from the configuration
 * files, for the enabled modules. Every field is mandatory.
 */

// NOLINTBEGIN(readability-identifier-length, modernize-use-constraints)

namespace ampersand::core::math {
NLOHMANN_DEFINE_TYPE_NON_INTRUSIVE(Vec2f, x, y)
NLOHMANN_DEFINE_TYPE_NON_INTRUSIVE(Vec2i, x, y)
NLOHMANN_DEFINE_TYPE_NON_INTRUSIVE(Vec2u, x, y)
NLOHMANN_DEFINE_TYPE_NON_INTRUSIVE(Vec3f, x, y, z)
NLOHMANN_DEFINE_TYPE_NON_INTRUSIVE(Rect<std::int32_t>, x, y, width, height)
NLOHMANN_DEFINE_TYPE_NON_INTRUSIVE(Rect<float>, x, y, width, height)
}  // namespace ampersand::core::math

namespace ampersand::core {
NLOHMANN_DEFINE_TYPE_NON_INTRUSIVE(Transform, position, scale, rotationDegrees,
                                   origin)
}  // namespace ampersand::core

#ifdef AMPERSAND_MODULE_RENDER
namespace ampersand::render {
NLOHMANN_DEFINE_TYPE_NON_INTRUSIVE(Color, r, g, b, a)
NLOHMANN_DEFINE_TYPE_NON_INTRUSIVE(ClearColor, value)
NLOHMANN_DEFINE_TYPE_NON_INTRUSIVE(Camera, center, size)
NLOHMANN_DEFINE_TYPE_NON_INTRUSIVE(WindowConfig, size, title, vsync)
NLOHMANN_DEFINE_TYPE_NON_INTRUSIVE(Animation, frames, frameDuration, loop)
NLOHMANN_DEFINE_TYPE_NON_INTRUSIVE(Rectangle, size, fill, outline,
                                   outlineThickness, layer)
NLOHMANN_DEFINE_TYPE_NON_INTRUSIVE(Circle, radius, fill, outline,
                                   outlineThickness, layer)
NLOHMANN_DEFINE_TYPE_NON_INTRUSIVE(Line, from, to, color, layer)
NLOHMANN_DEFINE_TYPE_NON_INTRUSIVE(AnimationParams, floats, bools)
}  // namespace ampersand::render
#endif

#ifdef AMPERSAND_MODULE_PHYSICS
namespace ampersand::physics {
NLOHMANN_DEFINE_TYPE_NON_INTRUSIVE(GravityForce, value)
NLOHMANN_DEFINE_TYPE_NON_INTRUSIVE(Velocity, value)
NLOHMANN_DEFINE_TYPE_NON_INTRUSIVE(Gravity, scale)
NLOHMANN_DEFINE_TYPE_NON_INTRUSIVE(BoxCollider, bounds)
}  // namespace ampersand::physics
#endif

#ifdef AMPERSAND_MODULE_INPUT
namespace ampersand::input {
// An unknown name reads as None (the first entry).
NLOHMANN_JSON_SERIALIZE_ENUM(KeyCode, {{KeyCode::None, nullptr},
                                       {KeyCode::A, "A"},
                                       {KeyCode::B, "B"},
                                       {KeyCode::C, "C"},
                                       {KeyCode::D, "D"},
                                       {KeyCode::E, "E"},
                                       {KeyCode::F, "F"},
                                       {KeyCode::G, "G"},
                                       {KeyCode::H, "H"},
                                       {KeyCode::I, "I"},
                                       {KeyCode::J, "J"},
                                       {KeyCode::K, "K"},
                                       {KeyCode::L, "L"},
                                       {KeyCode::M, "M"},
                                       {KeyCode::N, "N"},
                                       {KeyCode::O, "O"},
                                       {KeyCode::P, "P"},
                                       {KeyCode::Q, "Q"},
                                       {KeyCode::R, "R"},
                                       {KeyCode::S, "S"},
                                       {KeyCode::T, "T"},
                                       {KeyCode::U, "U"},
                                       {KeyCode::V, "V"},
                                       {KeyCode::W, "W"},
                                       {KeyCode::X, "X"},
                                       {KeyCode::Y, "Y"},
                                       {KeyCode::Z, "Z"},
                                       {KeyCode::Alpha0, "Alpha0"},
                                       {KeyCode::Alpha1, "Alpha1"},
                                       {KeyCode::Alpha2, "Alpha2"},
                                       {KeyCode::Alpha3, "Alpha3"},
                                       {KeyCode::Alpha4, "Alpha4"},
                                       {KeyCode::Alpha5, "Alpha5"},
                                       {KeyCode::Alpha6, "Alpha6"},
                                       {KeyCode::Alpha7, "Alpha7"},
                                       {KeyCode::Alpha8, "Alpha8"},
                                       {KeyCode::Alpha9, "Alpha9"},
                                       {KeyCode::UpArrow, "UpArrow"},
                                       {KeyCode::DownArrow, "DownArrow"},
                                       {KeyCode::LeftArrow, "LeftArrow"},
                                       {KeyCode::RightArrow, "RightArrow"},
                                       {KeyCode::Space, "Space"},
                                       {KeyCode::Return, "Return"},
                                       {KeyCode::Escape, "Escape"},
                                       {KeyCode::Tab, "Tab"},
                                       {KeyCode::Backspace, "Backspace"},
                                       {KeyCode::LeftShift, "LeftShift"},
                                       {KeyCode::RightShift, "RightShift"},
                                       {KeyCode::LeftControl, "LeftControl"},
                                       {KeyCode::RightControl, "RightControl"},
                                       {KeyCode::LeftAlt, "LeftAlt"},
                                       {KeyCode::RightAlt, "RightAlt"}})

NLOHMANN_JSON_SERIALIZE_ENUM(MouseButton, {{MouseButton::None, nullptr},
                                           {MouseButton::Left, "Left"},
                                           {MouseButton::Right, "Right"},
                                           {MouseButton::Middle, "Middle"}})

NLOHMANN_JSON_SERIALIZE_ENUM(JoystickButton,
                             {{JoystickButton::None, nullptr},
                              {JoystickButton::Button0, "Button0"},
                              {JoystickButton::Button1, "Button1"},
                              {JoystickButton::Button2, "Button2"},
                              {JoystickButton::Button3, "Button3"},
                              {JoystickButton::Button4, "Button4"},
                              {JoystickButton::Button5, "Button5"},
                              {JoystickButton::Button6, "Button6"},
                              {JoystickButton::Button7, "Button7"},
                              {JoystickButton::Button8, "Button8"},
                              {JoystickButton::Button9, "Button9"},
                              {JoystickButton::Button10, "Button10"},
                              {JoystickButton::Button11, "Button11"},
                              {JoystickButton::Button12, "Button12"},
                              {JoystickButton::Button13, "Button13"},
                              {JoystickButton::Button14, "Button14"},
                              {JoystickButton::Button15, "Button15"},
                              {JoystickButton::Button16, "Button16"},
                              {JoystickButton::Button17, "Button17"},
                              {JoystickButton::Button18, "Button18"},
                              {JoystickButton::Button19, "Button19"},
                              {JoystickButton::Button20, "Button20"},
                              {JoystickButton::Button21, "Button21"},
                              {JoystickButton::Button22, "Button22"},
                              {JoystickButton::Button23, "Button23"},
                              {JoystickButton::Button24, "Button24"},
                              {JoystickButton::Button25, "Button25"},
                              {JoystickButton::Button26, "Button26"},
                              {JoystickButton::Button27, "Button27"},
                              {JoystickButton::Button28, "Button28"},
                              {JoystickButton::Button29, "Button29"},
                              {JoystickButton::Button30, "Button30"},
                              {JoystickButton::Button31, "Button31"}})

NLOHMANN_JSON_SERIALIZE_ENUM(JoystickAxis, {{JoystickAxis::X, "X"},
                                            {JoystickAxis::Y, "Y"},
                                            {JoystickAxis::Z, "Z"},
                                            {JoystickAxis::R, "R"},
                                            {JoystickAxis::U, "U"},
                                            {JoystickAxis::V, "V"},
                                            {JoystickAxis::PovX, "PovX"},
                                            {JoystickAxis::PovY, "PovY"}})

/**
 * @brief Reads a binding from the name of a key ("Space"), a mouse button
 * ("Left") or a joystick button ("Button0"). null reads as no binding.
 */
inline void from_json(const nlohmann::json& data, Binding& binding) {
    if (data.is_null()) {
        binding = std::monostate{};
    } else if (data.get<KeyCode>() != KeyCode::None) {
        binding = data.get<KeyCode>();
    } else if (data.get<MouseButton>() != MouseButton::None) {
        binding = data.get<MouseButton>();
    } else if (data.get<JoystickButton>() != JoystickButton::None) {
        binding = data.get<JoystickButton>();
    } else {
        throw std::runtime_error("Unknown input '" + data.dump() + "'");
    }
}

/**
 * @brief Writes a binding as its name (null for no binding).
 */
inline void to_json(nlohmann::json& data, const Binding& binding) {
    data = nullptr;
    if (const auto* key = std::get_if<KeyCode>(&binding)) data = *key;
    if (const auto* button = std::get_if<MouseButton>(&binding)) data = *button;
    if (const auto* joystick = std::get_if<JoystickButton>(&binding))
        data = *joystick;
}

/**
 * @brief Reads the joystick axis of an Axis (null for none). JoystickAxis has
 * no None entry for an unknown name to read as, so the name is checked here.
 */
inline void from_json(const nlohmann::json& data,
                      std::optional<JoystickAxis>& axis) {
    axis =
        data.is_null() ? std::nullopt : std::optional(data.get<JoystickAxis>());
    if (axis && nlohmann::json(*axis) != data) {
        throw std::runtime_error("Unknown joystick axis '" + data.dump() + "'");
    }
}

/** @brief Writes the joystick axis of an Axis (null for none). */
inline void to_json(nlohmann::json& data,
                    const std::optional<JoystickAxis>& axis) {
    data = axis ? nlohmann::json(*axis) : nlohmann::json(nullptr);
}

NLOHMANN_DEFINE_TYPE_NON_INTRUSIVE(Axis, positive, negative, joystick, dead,
                                   invertJoystick)
}  // namespace ampersand::input
#endif

// NOLINTEND(readability-identifier-length, modernize-use-constraints)
