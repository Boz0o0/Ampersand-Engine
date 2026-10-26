#include <algorithm>
#include <ampersand/core/components/Transform.hpp>
#include <ampersand/core/ecs/CommandQueue.hpp>
#include <ampersand/core/ecs/EventQueue.hpp>
#include <ampersand/core/ecs/World.hpp>
#include <ampersand/core/math/Vec3.hpp>
#include <ampersand/core/resources/Clock.hpp>
#include <ampersand/scripting/components/Scripts.hpp>
#include <ampersand/scripting/systems/ScriptingSystem.hpp>
#include <stdexcept>

#ifdef AMPERSAND_MODULE_INPUT
#include <ampersand/input/resources/InputManager.hpp>
#endif

namespace ampersand::scripting {

ScriptingSystem::ScriptingSystem() {
    _lua.open_libraries(sol::lib::base, sol::lib::math, sol::lib::string,
                        sol::lib::table);
}

void ScriptingSystem::update(core::ecs::World& world) {
    if (!_bound) {
        bindTypes();
        bindWorld(world);
        bindInput(world);
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
    for (const core::ecs::Entity entity : world.owners<Scripts>()) {
        sync(world, entity, deltaTime);
        for (auto& instance : _instances[entity]) {
            if (!instance.started) {
                instance.started = true;
                run(world, instance, "start", deltaTime);
            }
            run(world, instance, "update", deltaTime);
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
    if (!result.valid() || result.get_type() != sol::type::table) {
        throw std::runtime_error("Script '" + path + "' must return a table");
    }
    Instance instance{.path = path, .self = result};
    instance.self["entity"] = entity;
    return instance;
}

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
    try {
        call(instance, hook, deltaTime);
    } catch (const std::exception& error) {
        report(world, instance.path, error);
    }
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

    _lua.new_usertype<core::math::Vec3f>(
        "Vec3",
        sol::constructors<core::math::Vec3f(),
                          core::math::Vec3f(float, float, float)>(),
        "x", &core::math::Vec3f::x, "y", &core::math::Vec3f::y, "z",
        &core::math::Vec3f::z);

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

    _lua.set_function("hasTransform", [&world](core::ecs::Entity entity) {
        return world.has<core::Transform>(entity);
    });

    _lua.set_function("destroy", [&world](core::ecs::Entity entity) {
        world.getResource<core::ecs::CommandQueue>().destroy(entity);
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
#endif
}

}  // namespace ampersand::scripting
