#include <algorithm>
#include <ampersand/core/ecs/CommandQueue.hpp>
#include <ampersand/core/ecs/EventQueue.hpp>
#include <ampersand/core/ecs/Events.hpp>
#include <ampersand/core/ecs/World.hpp>
#include <ampersand/core/hierarchy/WorldTransform.hpp>
#include <ampersand/core/math/Rect.hpp>
#include <ampersand/core/math/Vec2.hpp>
#include <ampersand/core/math/Vec3.hpp>
#include <ampersand/core/resources/Clock.hpp>
#include <ampersand/scripting/components/Scripts.hpp>
#include <ampersand/scripting/systems/ScriptingSystem.hpp>
#include <cstdint>
#include <stdexcept>
#include <utility>
#include <vector>

#ifdef AMPERSAND_MODULE_INPUT
#include <ampersand/input/resources/InputManager.hpp>
#endif

#ifdef AMPERSAND_MODULE_PHYSICS
#include <ampersand/physics/components/BoxCollider.hpp>
#include <ampersand/physics/components/Velocity.hpp>
#endif

#ifdef AMPERSAND_MODULE_RENDER
#include <ampersand/render/components/AnimationStateMachine.hpp>
#include <ampersand/render/components/Primitive.hpp>
#include <ampersand/render/components/Sprite.hpp>
#include <ampersand/render/components/Text.hpp>
#include <ampersand/render/resources/Camera.hpp>
#include <ampersand/render/resources/Renderer.hpp>
#endif

#ifdef AMPERSAND_MODULE_AUDIO
#include <ampersand/audio/components/AudioSource.hpp>
#endif

namespace ampersand::scripting {

void ScriptingSystem::call(Instance& instance, const std::string& hook,
                           float deltaTime) {
    if (instance.failed) {
        return;
    }
    sol::optional<sol::protected_function> function = instance.self[hook];
    if (!function) {
        return;
    }
    sol::protected_function_result result =
        (*function)(instance.self, deltaTime);
    if (!result.valid()) {
        sol::error error = result;
        instance.failed = true;
        throw std::runtime_error("Failed to call '" + hook + "' on script '" +
                                 instance.path + "': " + error.what());
    }
}

void ScriptingSystem::run(core::ecs::World& world, Instance& instance,
                          const std::string& hook, float deltaTime) {
    _running = instance.path;
    try {
        call(instance, hook, deltaTime);
    } catch (const std::exception& error) {
        report(world, instance.path, error);
    }
    _running.clear();
}

ScriptingSystem::ScriptingSystem() {
    _lua.open_libraries(sol::lib::base, sol::lib::math, sol::lib::string,
                        sol::lib::table);
}

void ScriptingSystem::update(core::ecs::World& world) {
    if (!_bound) {
        bindTypes();
        bindWorld(world);
        bindInput(world);
        bindPhysics(world);
        bindRender(world);
        bindAudio(world);
        _bound = true;
    }

    const float deltaTime = world.getResource<core::Clock>().dt;
    for (auto it = _instances.begin(); it != _instances.end();) {
        if (world.alive(it->first) && world.has<Scripts>(it->first)) {
            ++it;
            continue;
        }
        for (auto& instance : it->second) {
            run(world, instance, "onDestroy", deltaTime);
        }
        it = _instances.erase(it);
    }
    const std::vector<core::ecs::Entity> entities = world.owners<Scripts>();
    for (const core::ecs::Entity entity : entities) {
        if (!world.alive(entity) || !world.has<Scripts>(entity)) {
            continue;
        }
        sync(world, entity, deltaTime);
        for (auto& instance : _instances[entity]) {
            if (!instance.started) {
                instance.started = true;
                run(world, instance, "start", deltaTime);
            }
            run(world, instance, "update", deltaTime);
        }
    }
    applyDeferred(world);
}

void ScriptingSystem::defer(std::function<void(core::ecs::World&)> action) {
    _deferred.push_back({.path = _running, .action = std::move(action)});
}

void ScriptingSystem::applyDeferred(core::ecs::World& world) {
    for (auto& deferred : std::exchange(_deferred, {})) {
        try {
            deferred.action(world);
        } catch (const std::exception& error) {
            report(world, deferred.path, error);
        }
    }
}

void ScriptingSystem::sync(core::ecs::World& world, core::ecs::Entity entity,
                           float deltaTime) {
    const auto& paths = world.get<Scripts>(entity).scripts;
    auto& instances = _instances[entity];
    if (std::ranges::equal(paths, instances, {}, {}, &Instance::path)) {
        return;
    }
    for (auto& instance : instances) {
        run(world, instance, "onDestroy", deltaTime);
    }
    instances.clear();
    for (const auto& path : paths) {
        try {
            instances.push_back(createInstance(entity, path));
        } catch (const std::exception& error) {
            report(world, path, error);
            instances.push_back(Instance{.path = path, .failed = true});
        }
    }
}

ScriptingSystem::Instance ScriptingSystem::createInstance(
    core::ecs::Entity entity, const std::string& path) {
    sol::protected_function_result result = chunk(path)();
    if (!result.valid()) {
        sol::error error = result;
        throw std::runtime_error("Failed to run script '" + path +
                                 "': " + error.what());
    }
    if (result.get_type() != sol::type::table) {
        throw std::runtime_error("Script '" + path + "' must return a table");
    }
    Instance instance{.path = path, .self = result};
    instance.self["entity"] = entity;
    return instance;
}

void ScriptingSystem::report(core::ecs::World& world, const std::string& path,
                             const std::exception& error) {
    if (world.hasResource<core::ecs::EventQueue<ScriptError>>()) {
        world.getResource<core::ecs::EventQueue<ScriptError>>().send(
            {.path = path, .message = error.what()});
    }
}

sol::protected_function ScriptingSystem::chunk(const std::string& path) {
    auto cached = _chunks.find(path);
    if (cached == _chunks.end()) {
        sol::load_result loaded = _lua.load_file(path);
        if (!loaded.valid()) {
            sol::error error = loaded;
            throw std::runtime_error("Failed to load script '" + path +
                                     "': " + error.what());
        }
        sol::protected_function compiled = loaded;
        cached = _chunks.emplace(path, compiled.dump()).first;
    }
    sol::protected_function script = _lua.load(cached->second.as_string_view(),
                                               path, sol::load_mode::binary);
    sol::environment environment(_lua, sol::create, _lua.globals());
    sol::set_environment(environment, script);
    return script;
}

void ScriptingSystem::bindTypes() {
    _lua.new_usertype<core::ecs::Entity>(
        "Entity", sol::no_constructor, "index",
        sol::readonly(&core::ecs::Entity::index));
    _lua.new_usertype<core::math::Vec2f>(
        "Vec2",
        sol::constructors<core::math::Vec2f(),
                          core::math::Vec2f(float, float)>(),
        "x", &core::math::Vec2f::x, "y", &core::math::Vec2f::y);
    _lua.new_usertype<core::math::Vec3f>(
        "Vec3",
        sol::constructors<core::math::Vec3f(),
                          core::math::Vec3f(float, float, float)>(),
        "x", &core::math::Vec3f::x, "y", &core::math::Vec3f::y, "z",
        &core::math::Vec3f::z);
    _lua.new_usertype<core::math::Rect<float>>(
        "Rect",
        sol::constructors<core::math::Rect<float>(),
                          core::math::Rect<float>(float, float, float,
                                                  float)>(),
        "x", &core::math::Rect<float>::x, "y", &core::math::Rect<float>::y,
        "width", &core::math::Rect<float>::width, "height",
        &core::math::Rect<float>::height, "intersects",
        &core::math::Rect<float>::intersects);
    _lua.new_usertype<core::Transform>(
        "Transform", sol::no_constructor, "position",
        &core::Transform::position, "scale", &core::Transform::scale,
        "rotationDegrees", &core::Transform::rotationDegrees, "origin",
        &core::Transform::origin);
}

void ScriptingSystem::bindWorld(core::ecs::World& world) {
    _lua.set_function("getTransform",
                      [&world](core::ecs::Entity entity) -> core::Transform& {
                          return world.get<core::Transform>(entity);
                      });
    _lua.set_function("getWorldTransform", [&world](core::ecs::Entity entity) {
        return core::worldTransform(world, entity);
    });
    _lua.set_function("hasTransform", [&world](core::ecs::Entity entity) {
        return world.has<core::Transform>(entity);
    });
    _lua.set_function("kill", [&world](core::ecs::Entity entity) {
        if (!world.alive(entity)) {
            return;
        }
        if (world.hasResource<
                core::ecs::EventQueue<core::ecs::EntityDestroyed>>()) {
            auto& destroyed = world.getResource<
                core::ecs::EventQueue<core::ecs::EntityDestroyed>>();
            if (std::ranges::none_of(destroyed.read(),
                                     [entity](const auto& event) {
                                         return event.entity == entity;
                                     })) {
                destroyed.send({.entity = entity});
            }
        }
        world.getResource<core::ecs::CommandQueue>().destroy(entity);
    });
    _lua.set_function("destroy", [&world](core::ecs::Entity entity) {
        world.getResource<core::ecs::CommandQueue>().destroy(entity);
    });
    _lua.set_function(
        "getParent",
        [&world](core::ecs::Entity entity) -> sol::optional<core::ecs::Entity> {
            const core::ecs::Entity parent = world.parent(entity);
            if (parent == core::ecs::Entity{}) {
                return sol::nullopt;
            }
            return parent;
        });
    _lua.set_function("getChildren", [&world](core::ecs::Entity entity) {
        return sol::as_table(world.children(entity));
    });
}

void ScriptingSystem::bindInput([[maybe_unused]] core::ecs::World& world) {
#ifdef AMPERSAND_MODULE_INPUT
    sol::table inputTable = _lua.create_named_table("input");
    inputTable.set_function("getAxis", [&world](const std::string& name) {
        const auto& manager = world.getResource<input::InputManager>();
        return manager.getAxisRaw(name);
    });
    inputTable.set_function("getButton", [&world](const std::string& name) {
        const auto& manager = world.getResource<input::InputManager>();
        return manager.getButton(name);
    });
    inputTable.set_function("getButtonDown", [&world](const std::string& name) {
        const auto& manager = world.getResource<input::InputManager>();
        return manager.getButtonDown(name);
    });
    inputTable.set_function("getButtonUp", [&world](const std::string& name) {
        const auto& manager = world.getResource<input::InputManager>();
        return manager.getButtonUp(name);
    });
    const auto mouseButton = [](const std::string& name) {
        if (name == "Left") {
            return input::MouseButton::Left;
        }
        if (name == "Right") {
            return input::MouseButton::Right;
        }
        if (name == "Middle") {
            return input::MouseButton::Middle;
        }
        throw std::invalid_argument("Unknown mouse button '" + name + "'");
    };
    inputTable.set_function(
        "getMouseButton", [&world, mouseButton](const std::string& name) {
            const auto& manager = world.getResource<input::InputManager>();
            return manager.getMouseButton(mouseButton(name));
        });
    inputTable.set_function(
        "getMouseButtonDown", [&world, mouseButton](const std::string& name) {
            const auto& manager = world.getResource<input::InputManager>();
            return manager.getMouseButtonDown(mouseButton(name));
        });
    inputTable.set_function(
        "getMouseButtonUp", [&world, mouseButton](const std::string& name) {
            const auto& manager = world.getResource<input::InputManager>();
            return manager.getMouseButtonUp(mouseButton(name));
        });
    inputTable.set_function("getMousePosition", [&world] {
        const auto pixel = world.getResource<input::InputManager>().mouse();
        return core::math::Vec2f{static_cast<float>(pixel.x),
                                 static_cast<float>(pixel.y)};
    });
#endif
}

void ScriptingSystem::bindPhysics([[maybe_unused]] core::ecs::World& world) {
#ifdef AMPERSAND_MODULE_PHYSICS
    _lua.new_usertype<physics::BoxCollider>("BoxCollider", sol::no_constructor,
                                            "bounds",
                                            &physics::BoxCollider::bounds);
    _lua.set_function(
        "getBoxCollider",
        [&world](core::ecs::Entity entity) -> physics::BoxCollider& {
            return world.get<physics::BoxCollider>(entity);
        });
    _lua.set_function("hasBoxCollider", [&world](core::ecs::Entity entity) {
        return world.has<physics::BoxCollider>(entity);
    });
    _lua.set_function("getBounds", [&world](core::ecs::Entity entity) {
        return physics::worldBounds(core::worldTransform(world, entity),
                                    world.get<physics::BoxCollider>(entity));
    });
    _lua.new_usertype<physics::Velocity>("Velocity", sol::no_constructor,
                                         "value", &physics::Velocity::value);
    _lua.set_function("getVelocity",
                      [&world](core::ecs::Entity entity) -> physics::Velocity& {
                          return world.get<physics::Velocity>(entity);
                      });
    _lua.set_function("hasVelocity", [&world](core::ecs::Entity entity) {
        return world.has<physics::Velocity>(entity);
    });
    _lua.set_function("getColliders", [&world] {
        std::vector<core::ecs::Entity> colliders;
        for (const core::ecs::Entity entity :
             world.owners<physics::BoxCollider>()) {
            if (world.has<core::Transform>(entity)) {
                colliders.push_back(entity);
            }
        }
        return sol::as_table(std::move(colliders));
    });
#endif
}

void ScriptingSystem::bindRender([[maybe_unused]] core::ecs::World& world) {
#ifdef AMPERSAND_MODULE_RENDER
    _lua.new_usertype<render::Color>(
        "Color", sol::no_constructor, "r", &render::Color::r, "g",
        &render::Color::g, "b", &render::Color::b, "a", &render::Color::a);
    _lua.new_usertype<render::Text>(
        "Text", sol::no_constructor, "value", &render::Text::value, "size",
        &render::Text::size, "color", &render::Text::color, "layer",
        &render::Text::layer);
    _lua.set_function("getText",
                      [&world](core::ecs::Entity entity) -> render::Text& {
                          return world.get<render::Text>(entity);
                      });
    _lua.set_function("hasText", [&world](core::ecs::Entity entity) {
        return world.has<render::Text>(entity);
    });

    using IntRect = core::math::Rect<std::int32_t>;
    _lua.new_usertype<IntRect>(
        "IntRect",
        sol::constructors<IntRect(), IntRect(std::int32_t, std::int32_t,
                                             std::int32_t, std::int32_t)>(),
        "x", &IntRect::x, "y", &IntRect::y, "width", &IntRect::width, "height",
        &IntRect::height);
    _lua.new_usertype<render::Sprite>(
        "Sprite", sol::no_constructor, "source", &render::Sprite::source,
        "color", &render::Sprite::color, "layer", &render::Sprite::layer);
    _lua.set_function("getSprite",
                      [&world](core::ecs::Entity entity) -> render::Sprite& {
                          return world.get<render::Sprite>(entity);
                      });
    _lua.set_function("hasSprite", [&world](core::ecs::Entity entity) {
        return world.has<render::Sprite>(entity);
    });

    _lua.new_usertype<render::Rectangle>(
        "Rectangle", sol::no_constructor, "size", &render::Rectangle::size,
        "fill", &render::Rectangle::fill, "outline",
        &render::Rectangle::outline, "outlineThickness",
        &render::Rectangle::outlineThickness, "layer",
        &render::Rectangle::layer);
    _lua.set_function("getRectangle",
                      [&world](core::ecs::Entity entity) -> render::Rectangle& {
                          return world.get<render::Rectangle>(entity);
                      });
    _lua.set_function("hasRectangle", [&world](core::ecs::Entity entity) {
        return world.has<render::Rectangle>(entity);
    });
    _lua.new_usertype<render::Circle>(
        "Circle", sol::no_constructor, "radius", &render::Circle::radius,
        "fill", &render::Circle::fill, "outline", &render::Circle::outline,
        "outlineThickness", &render::Circle::outlineThickness, "layer",
        &render::Circle::layer);
    _lua.set_function("getCircle",
                      [&world](core::ecs::Entity entity) -> render::Circle& {
                          return world.get<render::Circle>(entity);
                      });
    _lua.set_function("hasCircle", [&world](core::ecs::Entity entity) {
        return world.has<render::Circle>(entity);
    });
    _lua.new_usertype<render::Line>(
        "Line", sol::no_constructor, "from", &render::Line::from, "to",
        &render::Line::to, "color", &render::Line::color, "layer",
        &render::Line::layer);
    _lua.set_function("getLine",
                      [&world](core::ecs::Entity entity) -> render::Line& {
                          return world.get<render::Line>(entity);
                      });
    _lua.set_function("hasLine", [&world](core::ecs::Entity entity) {
        return world.has<render::Line>(entity);
    });

    sol::table animation = _lua.create_named_table("animation");
    const auto machine =
        [&world](core::ecs::Entity entity) -> render::AnimationStateMachine& {
        return world.get<render::AnimationStateMachine>(entity);
    };
    animation.set_function(
        "setBool",
        [machine](core::ecs::Entity entity, const std::string& name,
                  bool value) { machine(entity).params.bools[name] = value; });
    animation.set_function(
        "setFloat", [machine](core::ecs::Entity entity, const std::string& name,
                              float value) {
            machine(entity).params.floats[name] = value;
        });
    animation.set_function("playOnce", [machine](core::ecs::Entity entity,
                                                 const std::string& action) {
        machine(entity).playOnce(action);
    });
    animation.set_function("isPlayingAction",
                           [machine](core::ecs::Entity entity) {
                               return machine(entity).isPlayingAction();
                           });
    animation.set_function("getState", [machine](core::ecs::Entity entity) {
        return machine(entity).current();
    });
#endif
}

void ScriptingSystem::bindAudio([[maybe_unused]] core::ecs::World& world) {
#ifdef AMPERSAND_MODULE_AUDIO
    _lua.new_usertype<audio::AudioSource>(
        "AudioSource", sol::no_constructor, "volume",
        &audio::AudioSource::volume, "loop", &audio::AudioSource::loop,
        "playing", &audio::AudioSource::playing);
    _lua.set_function(
        "getAudioSource",
        [&world](core::ecs::Entity entity) -> audio::AudioSource& {
            return world.get<audio::AudioSource>(entity);
        });
    _lua.set_function("hasAudioSource", [&world](core::ecs::Entity entity) {
        return world.has<audio::AudioSource>(entity);
    });
#endif
}

}  // namespace ampersand::scripting
