#pragma once

#include <ampersand/core/ecs/Entity.hpp>
#include <ampersand/core/ecs/ISystem.hpp>
#include <exception>
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

private:
    /** @brief One script of one entity, with its own `self` table. */
    struct Instance {
        std::string path;
        sol::table self;
        bool started{};
        bool failed{};
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
    static void run(core::ecs::World& world, Instance& instance,
                    const std::string& hook, float deltaTime);

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

    /** @brief Lua state. Declared first, so it is destroyed last. */
    sol::state _lua;
    /** @brief Bytecode of the compiled script files, by path. */
    std::unordered_map<std::string, sol::bytecode> _chunks;
    /** @brief Script instances of each entity. */
    std::unordered_map<core::ecs::Entity, std::vector<Instance>> _instances;
};

}  // namespace ampersand::scripting