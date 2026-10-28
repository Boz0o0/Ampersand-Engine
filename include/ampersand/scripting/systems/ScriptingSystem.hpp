#pragma once

#include <ampersand/core/ecs/Entity.hpp>
#include <ampersand/core/ecs/ISystem.hpp>
#include <exception>
#include <functional>
#include <sol/sol.hpp>
#include <string>
#include <unordered_map>
#include <vector>

namespace ampersand::scripting {

/**
 * @brief An error raised by a script (load, syntax or runtime error).
 */
struct ScriptError {
    /** @brief Path of the script file. */
    std::string path;
    /** @brief Description of the error. */
    std::string message;
};

/**
 * @brief Runs every script of every entity that has a Scripts component.
 */
class ScriptingSystem final : public core::ecs::ISystem {
public:
    ScriptingSystem();

    /**
     * @brief Runs every script of every entity that has a Scripts component.
     *
     * @param world World holding the entities.
     */
    void update(core::ecs::World& world) override;

    /**
     * @brief Lua state shared by every script, to add the bindings of the game
     * (its components and functions) with the sol2 API.
     *
     * Add them before the scripts using them run, and do not reuse the names
     * of the engine (getTransform, destroy, input, Vec3...): they would
     * replace them.
     */
    sol::state& lua() { return _lua; }

    /**
     * @brief Runs an action at the end of the current update, after every
     * script ran.
     *
     * For the bindings changing the World too much to run while the scripts
     * are iterated. If the action throws, the error is sent as a ScriptError
     * of the script that called the binding.
     *
     * @param action Function applied to the World.
     */
    void defer(std::function<void(core::ecs::World&)> action);

private:
    /** @brief One script of one entity, with its own `self` table. */
    struct Instance {
        std::string path;
        sol::table self;
        bool started{};
        bool failed{};
    };

    /** @brief An action given to defer, with the script that asked for it. */
    struct Deferred {
        std::string path;
        std::function<void(core::ecs::World&)> action;
    };

    /** @brief Matches the instances of an entity with its Scripts component.
     *
     * Keeps the instances of the scripts still listed, creates the new ones
     * and calls onDestroy on the removed ones.
     *
     * @param world World holding the entity.
     * @param entity Entity whose instances are synced.
     * @param deltaTime Time since the last frame.
     */
    void sync(core::ecs::World& world, core::ecs::Entity entity,
              float deltaTime);

    /** @brief Creates a new script instance for an entity.
     *
     * @param entity Entity to attach the script to.
     * @param path Path to the script file.
     * @return The new script instance.
     */
    Instance createInstance(core::ecs::Entity entity, const std::string& path);

    /** @brief Calls a function on a script instance.
     *
     * @param instance Script instance to call the function on.
     * @param hook Name of the function to call.
     * @param deltaTime Time since the last frame.
     */
    static void call(Instance& instance, const std::string& hook,
                     float deltaTime);

    /** @brief Calls a function on a script instance, reporting its error.
     *
     * @param world World holding the error queue.
     * @param instance Script instance to call the function on.
     * @param hook Name of the function to call.
     * @param deltaTime Time since the last frame.
     */
    void run(core::ecs::World& world, Instance& instance,
             const std::string& hook, float deltaTime);

    /** @brief Applies the actions given to defer, reporting their errors.
     *
     * @param world World the actions are applied to.
     */
    void applyDeferred(core::ecs::World& world);

    /** @brief Sends an error to the EventQueue<ScriptError>, if it exists.
     *
     * @param world World holding the error queue.
     * @param path Path to the script file.
     * @param error Error raised by the script.
     */
    static void report(core::ecs::World& world, const std::string& path,
                       const std::exception& error);

    /** @brief Returns a new function running a script file, with its own
     * environment: the globals it sets are only seen by this instance.
     *
     * The file is compiled once, then loaded again from its bytecode.
     *
     * @param path Path to the script file.
     * @return The script function, ready to be called.
     * @throw std::runtime_error If the script file cannot be loaded or
     * compiled.
     */
    sol::protected_function chunk(const std::string& path);

    /** @brief Binds the types to the Lua state. */
    void bindTypes();

    /** @brief Binds the world to the Lua state. */
    void bindWorld(core::ecs::World& world);

    /** @brief Binds the input to the Lua state: the axes, the buttons and the
     * mouse. */
    void bindInput([[maybe_unused]] core::ecs::World& world);

    /** @brief Binds Velocity, BoxCollider and the bounds helpers, when the
     * physics module is on. */
    void bindPhysics([[maybe_unused]] core::ecs::World& world);

    /** @brief Binds Text, Sprite, IntRect, Color and the animation table, when
     * the render module is on. */
    void bindRender([[maybe_unused]] core::ecs::World& world);

    /** @brief Binds AudioSource, when the audio module is on. */
    void bindAudio([[maybe_unused]] core::ecs::World& world);

    /** @brief Lua state. Declared first, so it is destroyed last. */
    sol::state _lua;
    /** @brief Bytecode of the compiled script files, by path. */
    std::unordered_map<std::string, sol::bytecode> _chunks;
    /** @brief Script instances of each entity. */
    std::unordered_map<core::ecs::Entity, std::vector<Instance>> _instances;
    /** @brief Indicates whether the scripting system has been bound. */
    bool _bound{false};
    /** @brief Actions given to defer, applied at the end of the update. */
    std::vector<Deferred> _deferred;
    /** @brief Path of the script running, empty between two scripts. */
    std::string _running;
};

}  // namespace ampersand::scripting