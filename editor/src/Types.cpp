#include <algorithm>
#include <ampersand/scene/serialization/Serializers.hpp>
#include <array>
#include <charconv>
#include <editor/Types.hpp>
#include <string>

#ifdef AMPERSAND_MODULE_RENDER
#include <ampersand/render/components/Sprite.hpp>
#include <ampersand/render/components/Text.hpp>
#endif

#ifdef AMPERSAND_MODULE_AUDIO
#include <ampersand/audio/components/AudioSource.hpp>
#include <ampersand/audio/resources/SceneMusic.hpp>
#endif

#ifdef AMPERSAND_MODULE_SCRIPTING
#include <ampersand/scripting/components/Scripts.hpp>
#endif

namespace ampersand::editor {

const TypeTable& TypeTable::get() {
    static const TypeTable table;
    return table;
}

TypeTable::TypeTable() {
    this->_fields["scenes"].entryDefault = std::string();
    this->addCore();
    this->addRender();
    this->addPhysics();
    this->addAudio();
    this->addInput();
    this->addScripting();
}

void TypeTable::addCore() {
    this->_components.push_back({
        .name = "Transform",
        .defaults = toJson(core::Transform{}),
    });
}

void TypeTable::addRender() {
#ifdef AMPERSAND_MODULE_RENDER
    const render::Sprite sprite{};
    const render::Text text{};
    const render::WindowConfig window{};

    this->_components.push_back(
        {.name = "Animation", .defaults = toJson(render::Animation{})});
    this->_components.push_back(
        {.name = "Rectangle", .defaults = toJson(render::Rectangle{})});
    this->_components.push_back(
        {.name = "Circle", .defaults = toJson(render::Circle{})});
    this->_components.push_back(
        {.name = "Line", .defaults = toJson(render::Line{})});
    this->_components.push_back({
        .name = "Sprite",
        .defaults =
            {
                {"texture", ""},
                {"source", toJson(sprite.source)},
                {"color", toJson(sprite.color)},
                {"layer", sprite.layer},
            },
    });
    this->_components.push_back({
        .name = "Text",
        .defaults =
            {
                {"font", ""},
                {"value", text.value},
                {"size", text.size},
                {"color", toJson(text.color)},
                {"layer", text.layer},
            },
    });
    // The initial state must exist, so the default has one empty state
    this->_components.push_back({
        .name = "AnimationStateMachine",
        .defaults =
            {
                {"initial", "idle"},
                {"states", {{"idle", toJson(render::Animation{})}}},
                {"params", toJson(render::AnimationParams{})},
                {"transitions", nlohmann::ordered_json::array()},
            },
    });
    this->_fields["states"].entryDefault = toJson(render::Animation{});
    this->_fields["floats"].entryDefault = 0.0;
    this->_fields["bools"].entryDefault = false;
    this->_fields["frames"].itemDefault =
        toJson(core::math::Rect<std::int32_t>{});

    this->_resources.push_back(
        {.name = "ClearColor", .defaults = toJson(render::ClearColor{})});
    this->_resources.push_back(
        {.name = "Camera", .defaults = toJson(render::Camera{})});
    this->_resources.push_back({
        .name = "Renderer",
        .defaults =
            {
                {"backend", "sfml"},
                {"size", toJson(window.size)},
                {"title", window.title},
                {"vsync", window.vsync},
            },
        .configOnly = true,
    });
#endif
}

void TypeTable::addPhysics() {
#ifdef AMPERSAND_MODULE_PHYSICS
    this->_components.push_back(
        {.name = "Velocity", .defaults = toJson(physics::Velocity{})});
    this->_components.push_back(
        {.name = "Gravity", .defaults = toJson(physics::Gravity{})});
    this->_components.push_back(
        {.name = "BoxCollider", .defaults = toJson(physics::BoxCollider{})});

    this->_resources.push_back(
        {.name = "GravityForce", .defaults = toJson(physics::GravityForce{})});
#endif
}

void TypeTable::addAudio() {
#ifdef AMPERSAND_MODULE_AUDIO
    const audio::AudioSource source{};
    const audio::SceneMusic music{};

    // The loader takes "sound" or "music": the default plays a sound
    this->_components.push_back({
        .name = "AudioSource",
        .defaults =
            {
                {"sound", ""},
                {"volume", source.volume},
                {"loop", source.loop},
                {"playing", source.playing},
            },
    });

    this->_resources.push_back({
        .name = "SceneMusic",
        .defaults =
            {
                {"music", ""},
                {"volume", music.volume},
                {"loop", music.loop},
                {"playing", music.playing},
            },
    });
    this->_resources.push_back({
        .name = "AudioListener",
        .defaults = {{"backend", "sfml"}, {"volume", 100.0F}},
        .configOnly = true,
    });
#endif
}

void TypeTable::addInput() {
#ifdef AMPERSAND_MODULE_INPUT
    this->_resources.push_back({
        .name = "InputManager",
        .defaults =
            {
                {"axes", nlohmann::ordered_json::object()},
                {"buttons", nlohmann::ordered_json::object()},
            },
        .configOnly = true,
    });

    std::vector<std::string> bindings;
    addEnumNames<input::KeyCode>(bindings);
    addEnumNames<input::MouseButton>(bindings);
    addEnumNames<input::JoystickButton>(bindings);
    std::vector<std::string> joystickAxes;
    addEnumNames<input::JoystickAxis>(joystickAxes);
    // The Binding to_json only takes nlohmann::json
    nlohmann::ordered_json axis =
        nlohmann::ordered_json::parse(nlohmann::json(input::Axis{}).dump());
    roundFloats(axis);

    this->_fields["axes"].entryDefault = std::move(axis);
    this->_fields["positive"].choices = bindings;
    this->_fields["negative"].choices = bindings;
    this->_fields["buttons"] = {
        .entryDefault = nlohmann::ordered_json::array(),
        .choices = std::move(bindings),
    };
    this->_fields["joystick"].choices = std::move(joystickAxes);
#endif
}

void TypeTable::addScripting() {
#ifdef AMPERSAND_MODULE_SCRIPTING
    this->_fields["scripts"].itemDefault = std::string();
    this->_components.push_back({
        .name = "Scripts",
        .defaults = {{"scripts", nlohmann::ordered_json::array()}},
    });
#endif
}

const FieldInfo& TypeTable::getField(std::string_view key) const {
    static const FieldInfo unknown;
    const auto found = this->_fields.find(key);
    return found != this->_fields.end() ? found->second : unknown;
}

nlohmann::ordered_json TypeTable::getComponentDefaults(
    std::string_view name) const {
    const auto type =
        std::ranges::find(this->_components, name, &TypeInfo::name);
    return type != this->_components.end() ? type->defaults
                                           : nlohmann::ordered_json::object();
}

void TypeTable::roundFloats(nlohmann::ordered_json& data) {
    if (data.is_number_float()) {
        std::array<char, 32> text{};
        const auto value = static_cast<float>(data.get<double>());
        const auto result =
            std::to_chars(text.data(), text.data() + text.size(), value);
        data = std::stod(std::string(text.data(), result.ptr));
    } else if (data.is_structured()) {
        for (auto& child : data) {
            roundFloats(child);
        }
    }
}

}  // namespace ampersand::editor
