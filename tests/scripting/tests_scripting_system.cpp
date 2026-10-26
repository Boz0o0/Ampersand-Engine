#include <gtest/gtest.h>

#include <algorithm>
#include <ampersand/core/components/Transform.hpp>
#include <ampersand/core/ecs/CommandQueue.hpp>
#include <ampersand/core/ecs/ECSExceptions.hpp>
#include <ampersand/core/ecs/EventQueue.hpp>
#include <ampersand/core/ecs/World.hpp>
#include <ampersand/core/resources/Clock.hpp>
#include <ampersand/scripting/components/Scripts.hpp>
#include <ampersand/scripting/systems/ScriptingSystem.hpp>
#include <filesystem>
#include <fstream>
#include <stdexcept>
#include <string>
#include <vector>

#ifdef AMPERSAND_MODULE_INPUT
#include <ampersand/input/Event.hpp>
#include <ampersand/input/resources/InputManager.hpp>
#endif

#ifdef AMPERSAND_MODULE_PHYSICS
#include <ampersand/physics/components/BoxCollider.hpp>
#endif

#ifdef AMPERSAND_MODULE_RENDER
#include <ampersand/render/components/AnimationStateMachine.hpp>
#endif

#ifdef AMPERSAND_MODULE_AUDIO
#include <ampersand/audio/components/AudioSource.hpp>
#endif

using ampersand::core::Clock;
using ampersand::core::Transform;
using ampersand::core::ecs::CommandQueue;
using ampersand::core::ecs::Entity;
using ampersand::core::ecs::EventQueue;
using ampersand::core::ecs::ResourceNotFound;
using ampersand::core::ecs::World;
using ampersand::scripting::ScriptError;
using ampersand::scripting::ScriptingSystem;
using ampersand::scripting::Scripts;

/**
 * @brief Writes the scripts in a temporary folder and runs the frames.
 *
 * Most scripts tell what they saw by raising an error: its message is read
 * back from the EventQueue<ScriptError>.
 */
class ScriptingSystemTest : public ::testing::Test {
protected:
    void SetUp() override {
        _root = std::filesystem::temp_directory_path() /
                ("ampersand_scripting_" +
                 std::string(::testing::UnitTest::GetInstance()
                                 ->current_test_info()
                                 ->name()));
        std::filesystem::remove_all(_root);
        std::filesystem::create_directories(_root);
        _world.addResource<Clock>(Clock{.dt = 0.5F});
        _world.addResource<EventQueue<ScriptError>>();
        _world.addResource<CommandQueue>();
    }

    void TearDown() override { std::filesystem::remove_all(_root); }

    /** @brief Writes a script under the root and returns its path. */
    [[nodiscard]] std::string script(const std::string& name,
                                     const std::string& code) const {
        const auto path = (_root / name).string();
        std::ofstream(path) << code;
        return path;
    }

    /** @brief Path of a script under the root that is never written. */
    [[nodiscard]] std::string missing(const std::string& name) const {
        return (_root / name).string();
    }

    /** @brief Creates an entity running the given scripts. */
    Entity spawn(std::vector<std::string> paths) {
        const Entity entity = _world.create();
        _world.add(entity, Scripts{std::move(paths)});
        return entity;
    }

    /** @brief Runs one update and returns the messages of its errors. */
    std::vector<std::string> frame() {
        _system.update(_world);
        auto& queue = _world.getResource<EventQueue<ScriptError>>();
        std::vector<std::string> messages;
        for (const auto& error : queue.read()) {
            messages.push_back(error.message);
        }
        queue.clear();
        return messages;
    }

    /** @brief Number of messages containing the text. */
    static long count(const std::vector<std::string>& messages,
                      const std::string& text) {
        return std::ranges::count_if(messages, [&](const std::string& message) {
            return message.find(text) != std::string::npos;
        });
    }

    std::filesystem::path _root;
    World _world;
    ScriptingSystem _system;
};

TEST_F(ScriptingSystemTest, StartRunsOnceBeforeUpdate) {
    spawn({
        script("start.lua", R"(
        local S = {}
        function S:start() self.starts = (self.starts or 0) + 1 end
        function S:update() error("starts=" .. self.starts) end
        return S
    )"),
    });

    const auto messages = frame();
    EXPECT_EQ(count(messages, "starts=1"), 1);
}

TEST_F(ScriptingSystemTest, UpdateRunsEveryFrameWithDt) {
    spawn({
        script("update.lua", R"(
        local S = {}
        function S:update(dt)
            self.time = (self.time or 0) + dt
            if self.time >= 1.5 then error("time=" .. self.time) end
        end
        return S
    )"),
    });

    EXPECT_TRUE(frame().empty());
    EXPECT_TRUE(frame().empty());
    EXPECT_EQ(count(frame(), "time=1.5"), 1);
}

TEST_F(ScriptingSystemTest, SelfKnowsItsEntity) {
    spawn({
        script("entity.lua", R"(
        local S = {}
        function S:update() error("entity=" .. tostring(self.entity ~= nil)) end
        return S
    )"),
    });

    EXPECT_EQ(count(frame(), "entity=true"), 1);
}

TEST_F(ScriptingSystemTest, EachEntityHasItsOwnInstance) {
    const auto path = script("counter.lua", R"(
        local S = {}
        function S:update()
            self.n = (self.n or 0) + 1
            if self.n == 2 then error("n=2") end
        end
        return S
    )");
    spawn({path});
    spawn({path});

    EXPECT_TRUE(frame().empty());
    EXPECT_EQ(count(frame(), "n=2"), 2);
}

TEST_F(ScriptingSystemTest, GlobalsAreKeptPerInstance) {
    spawn({
        script("writer.lua", R"(
        local S = {}
        function S:start() shared = 1 end
        return S
    )"),
        script("reader.lua", R"(
        local S = {}
        function S:update() error("shared=" .. tostring(shared)) end
        return S
    )"),
    });

    EXPECT_EQ(count(frame(), "shared=nil"), 1);
}

TEST_F(ScriptingSystemTest, InstancesOfTheSameFileDoNotShareGlobals) {
    const auto path = script("global.lua", R"(
        local S = {}
        function S:update()
            count = (count or 0) + 1
            if count == 2 then error("count=2") end
        end
        return S
    )");
    spawn({path});
    spawn({path});

    EXPECT_TRUE(frame().empty());
    EXPECT_EQ(count(frame(), "count=2"), 2);
}

TEST_F(ScriptingSystemTest, LibrariesAreAvailable) {
    spawn({
        script("libraries.lua", R"(
        local S = {}
        function S:update()
            error("floor=" .. math.floor(2.5) .. " upper=" .. string.upper("a"))
        end
        return S
    )"),
    });

    EXPECT_EQ(count(frame(), "floor=2 upper=A"), 1);
}

TEST_F(ScriptingSystemTest, OnDestroyRunsOnceWhenTheEntityIsDestroyed) {
    const Entity entity = spawn({
        script("destroy.lua", R"(
        local S = {}
        function S:onDestroy() error("destroyed") end
        return S
    )"),
    });
    EXPECT_TRUE(frame().empty());

    _world.destroy(entity);

    EXPECT_EQ(count(frame(), "destroyed"), 1);
    EXPECT_TRUE(frame().empty());
}

TEST_F(ScriptingSystemTest, OnDestroyRunsWhenScriptsIsRemoved) {
    const Entity entity = spawn({
        script("destroy.lua", R"(
        local S = {}
        function S:onDestroy() error("destroyed") end
        return S
    )"),
    });
    EXPECT_TRUE(frame().empty());

    _world.remove<Scripts>(entity);

    EXPECT_EQ(count(frame(), "destroyed"), 1);
    EXPECT_TRUE(frame().empty());
}

TEST_F(ScriptingSystemTest, ChangingTheListRebuildsTheInstances) {
    const Entity entity = spawn({
        script("first.lua", R"(
        local S = {}
        function S:onDestroy() error("first destroyed") end
        return S
    )"),
    });
    EXPECT_TRUE(frame().empty());

    _world.get<Scripts>(entity).scripts.push_back(script("second.lua", R"(
        local S = {}
        function S:start() error("second started") end
        return S
    )"));

    const auto messages = frame();
    EXPECT_EQ(count(messages, "first destroyed"), 1);
    EXPECT_EQ(count(messages, "second started"), 1);
}

TEST_F(ScriptingSystemTest, MissingFileIsReportedOnce) {
    spawn({missing("missing.lua")});

    EXPECT_EQ(count(frame(), "Failed to load script"), 1);
    EXPECT_TRUE(frame().empty());
}

TEST_F(ScriptingSystemTest, SyntaxErrorIsReported) {
    spawn({script("syntax.lua", "local x = = 1")});

    EXPECT_EQ(count(frame(), "Failed to load script"), 1);
}

TEST_F(ScriptingSystemTest, ScriptNotReturningATableIsReported) {
    spawn({script("number.lua", "return 42")});

    EXPECT_EQ(count(frame(), "must return a table"), 1);
}

TEST_F(ScriptingSystemTest, ErrorWhenTheFileRunsKeepsTheLuaMessage) {
    spawn({script("top_level.lua", R"(error("top level boom"))")});

    const auto messages = frame();
    EXPECT_EQ(count(messages, "Failed to run script"), 1);
    EXPECT_EQ(count(messages, "top level boom"), 1);
}

TEST_F(ScriptingSystemTest, ScriptsAddedDuringTheLoopStartNextUpdate) {
    const auto child = script("child.lua", R"(
        local S = {}
        function S:start() error("child started") end
        return S
    )");
    _system.lua().set_function("addScripted",
                               [this, child = std::string(child)] {
                                   const Entity entity = _world.create();
                                   _world.add(entity, Scripts{{child}});
                               });
    spawn({
        script("parent.lua", R"(
        local S = {}
        function S:start() addScripted() end
        return S
    )"),
    });

    EXPECT_TRUE(frame().empty());
    EXPECT_EQ(count(frame(), "child started"), 1);
}

TEST_F(ScriptingSystemTest, ScriptsRemovedDuringTheLoopAreSkipped) {
    const Entity first = spawn({
        script("remover.lua", R"(
        local S = {}
        function S:update() removeOther() end
        return S
    )"),
    });
    const Entity second = spawn({
        script("removed.lua", R"(
        local S = {}
        function S:update() error("removed ran") end
        return S
    )"),
    });
    ASSERT_EQ(_world.owners<Scripts>().front(), first);
    _system.lua().set_function(
        "removeOther", [this, second] { _world.remove<Scripts>(second); });

    EXPECT_EQ(count(frame(), "removed ran"), 0);
}

TEST_F(ScriptingSystemTest, FailedScriptIsTurnedOffAndOthersKeepRunning) {
    spawn({
        script("broken.lua", R"(
        local S = {}
        function S:update() error("broken") end
        return S
    )"),
        script("counter.lua", R"(
        local S = {}
        function S:update()
            self.n = (self.n or 0) + 1
            if self.n == 3 then error("n=3") end
        end
        return S
    )"),
    });

    EXPECT_EQ(count(frame(), "broken"), 1);
    EXPECT_TRUE(frame().empty());
    EXPECT_EQ(count(frame(), "n=3"), 1);
}

TEST_F(ScriptingSystemTest, ErrorKeepsThePathOfTheScript) {
    const auto path = script("path.lua", R"(
        local S = {}
        function S:update() error("boom") end
        return S
    )");
    spawn({path});

    _system.update(_world);

    const auto& errors = _world.getResource<EventQueue<ScriptError>>().read();
    ASSERT_EQ(errors.size(), 1U);
    EXPECT_EQ(errors.front().path, path);
}

TEST_F(ScriptingSystemTest, ErrorsAreDroppedWithoutTheQueue) {
    _world.removeResource<EventQueue<ScriptError>>();
    spawn({missing("missing.lua")});

    EXPECT_NO_THROW(_system.update(_world));
}

TEST_F(ScriptingSystemTest, ThrowsWithoutAClock) {
    _world.removeResource<Clock>();

    EXPECT_THROW(_system.update(_world), ResourceNotFound);
}

TEST_F(ScriptingSystemTest, EntityIndexIsReadable) {
    const Entity entity = spawn({
        script("index.lua", R"(
        local S = {}
        function S:update() error("index=" .. self.entity.index) end
        return S
    )"),
    });

    EXPECT_EQ(count(frame(), "index=" + std::to_string(entity.index)), 1);
}

TEST_F(ScriptingSystemTest, Vec3CanBeCreated) {
    spawn({
        script("vec3.lua", R"(
        local S = {}
        function S:update()
            local v = Vec3.new(1, 2, 3)
            error("sum=" .. (v.x + v.y + v.z))
        end
        return S
    )"),
    });

    EXPECT_EQ(count(frame(), "sum=6"), 1);
}

TEST_F(ScriptingSystemTest, ScriptMovesTheTransform) {
    const Entity entity = spawn({
        script("move.lua", R"(
        local S = {}
        function S:update()
            local t = getTransform(self.entity)
            t.position.x = t.position.x + 10
        end
        return S
    )"),
    });
    _world.add(entity, Transform{});

    EXPECT_TRUE(frame().empty());
    EXPECT_FLOAT_EQ(_world.get<Transform>(entity).position.x, 10.0F);
    EXPECT_TRUE(frame().empty());
    EXPECT_FLOAT_EQ(_world.get<Transform>(entity).position.x, 20.0F);
}

TEST_F(ScriptingSystemTest, ScriptReplacesAVec3) {
    const Entity entity = spawn({
        script("scale.lua", R"(
        local S = {}
        function S:update() getTransform(self.entity).scale = Vec3.new(2, 3, 4) end
        return S
    )"),
    });
    _world.add(entity, Transform{});

    EXPECT_TRUE(frame().empty());
    const auto& scale = _world.get<Transform>(entity).scale;
    EXPECT_FLOAT_EQ(scale.x, 2.0F);
    EXPECT_FLOAT_EQ(scale.y, 3.0F);
    EXPECT_FLOAT_EQ(scale.z, 4.0F);
}

TEST_F(ScriptingSystemTest, HasTransformTellsIfThereIsOne) {
    const auto path = script("has.lua", R"(
        local S = {}
        function S:update() error("has=" .. tostring(hasTransform(self.entity))) end
        return S
    )");
    const Entity withTransform = spawn({path});
    _world.add(withTransform, Transform{});
    spawn({path});

    const auto messages = frame();
    EXPECT_EQ(count(messages, "has=true"), 1);
    EXPECT_EQ(count(messages, "has=false"), 1);
}

TEST_F(ScriptingSystemTest, GetTransformWithoutOneIsAScriptError) {
    spawn({
        script("missing_transform.lua", R"(
        local S = {}
        function S:update() getTransform(self.entity) end
        return S
    )"),
        script("counter.lua", R"(
        local S = {}
        function S:update()
            self.n = (self.n or 0) + 1
            if self.n == 2 then error("n=2") end
        end
        return S
    )"),
    });

    EXPECT_EQ(count(frame(), "Component not found"), 1);
    EXPECT_EQ(count(frame(), "n=2"), 1);
}

TEST_F(ScriptingSystemTest, DestroyWaitsForTheCommandQueue) {
    const Entity entity = spawn({
        script("destroy_self.lua", R"(
        local S = {}
        function S:update() destroy(self.entity) end
        function S:onDestroy() error("destroyed") end
        return S
    )"),
    });

    EXPECT_TRUE(frame().empty());
    EXPECT_TRUE(_world.alive(entity));

    _world.getResource<CommandQueue>().apply(_world);
    EXPECT_FALSE(_world.alive(entity));

    EXPECT_EQ(count(frame(), "destroyed"), 1);
    EXPECT_TRUE(frame().empty());
}

/** @brief Component of a game, bound by the tests through lua(). */
struct Health {
    int value{};
};

TEST_F(ScriptingSystemTest, GameCanAddFunctions) {
    _system.lua().set_function("twice", [](int value) { return value * 2; });
    spawn({
        script("twice.lua", R"(
        local S = {}
        function S:update() error("twice=" .. twice(21)) end
        return S
    )"),
    });

    EXPECT_EQ(count(frame(), "twice=42"), 1);
}

TEST_F(ScriptingSystemTest, GameCanBindItsComponents) {
    _system.lua().new_usertype<Health>("Health", sol::no_constructor, "value",
                                       &Health::value);
    _system.lua().set_function("getHealth", [this](Entity entity) -> Health& {
        return _world.get<Health>(entity);
    });
    const Entity entity = spawn({
        script("damage.lua", R"(
        local S = {}
        function S:update() getHealth(self.entity).value = getHealth(self.entity).value - 1 end
        return S
    )"),
    });
    _world.add(entity, Health{.value = 3});

    EXPECT_TRUE(frame().empty());
    EXPECT_EQ(_world.get<Health>(entity).value, 2);
}

TEST_F(ScriptingSystemTest, DeferredActionRunsAtTheEndOfTheUpdate) {
    bool ran = false;
    _system.defer([&ran](World& /*world*/) { ran = true; });
    EXPECT_FALSE(ran);

    EXPECT_TRUE(frame().empty());
    EXPECT_TRUE(ran);
}

TEST_F(ScriptingSystemTest, DeferredFromLuaRunsAfterEveryScript) {
    bool flag = false;
    _system.lua().set_function("later", [this, &flag] {
        _system.defer([&flag](World& /*world*/) { flag = true; });
    });
    _system.lua().set_function("flag", [&flag] { return flag; });
    spawn({
        script("later.lua", R"(
        local S = {}
        function S:update() later() end
        return S
    )"),
        script("check.lua", R"(
        local S = {}
        function S:update() error("flag=" .. tostring(flag())) end
        return S
    )"),
    });

    EXPECT_EQ(count(frame(), "flag=false"), 1);
    EXPECT_TRUE(flag);
}

TEST_F(ScriptingSystemTest, DeferredErrorIsReportedForItsScript) {
    _system.lua().set_function("failLater", [this] {
        _system.defer(
            [](World& /*world*/) { throw std::runtime_error("later failed"); });
    });
    const auto path = script("fail_later.lua", R"(
        local S = {}
        function S:start() failLater() end
        return S
    )");
    spawn({path});

    _system.update(_world);

    const auto& errors = _world.getResource<EventQueue<ScriptError>>().read();
    ASSERT_EQ(errors.size(), 1U);
    EXPECT_EQ(errors.front().path, path);
    EXPECT_EQ(errors.front().message, "later failed");
}

TEST_F(ScriptingSystemTest, ActionDeferredByAnActionRunsNextUpdate) {
    bool second = false;
    _system.defer([this, &second](World& /*world*/) {
        _system.defer([&second](World& /*world*/) { second = true; });
    });

    EXPECT_TRUE(frame().empty());
    EXPECT_FALSE(second);
    EXPECT_TRUE(frame().empty());
    EXPECT_TRUE(second);
}

TEST_F(ScriptingSystemTest, GetChildrenListsThemInOrder) {
    const Entity parent = spawn({
        script("children.lua", R"(
        local S = {}
        function S:update()
            local children = getChildren(self.entity)
            error("count=" .. #children .. " first=" .. children[1].index
                  .. " second=" .. children[2].index)
        end
        return S
    )"),
    });
    const Entity first = _world.create();
    const Entity second = _world.create();
    _world.setParent(first, parent);
    _world.setParent(second, parent);

    EXPECT_EQ(count(frame(), "count=2 first=" + std::to_string(first.index) +
                                 " second=" + std::to_string(second.index)),
              1);
}

TEST_F(ScriptingSystemTest, GetChildrenOfAnEntityWithoutChildrenIsEmpty) {
    spawn({
        script("no_children.lua", R"(
        local S = {}
        function S:update() error("count=" .. #getChildren(self.entity)) end
        return S
    )"),
    });

    EXPECT_EQ(count(frame(), "count=0"), 1);
}

TEST_F(ScriptingSystemTest, GetParentGivesTheParent) {
    const Entity child = spawn({
        script("parent.lua", R"(
        local S = {}
        function S:update() error("parent=" .. getParent(self.entity).index) end
        return S
    )"),
    });
    const Entity parent = _world.create();
    _world.setParent(child, parent);

    EXPECT_EQ(count(frame(), "parent=" + std::to_string(parent.index)), 1);
}

TEST_F(ScriptingSystemTest, GetParentOfARootIsNil) {
    spawn({
        script("root.lua", R"(
        local S = {}
        function S:update() error("parent=" .. tostring(getParent(self.entity))) end
        return S
    )"),
    });

    EXPECT_EQ(count(frame(), "parent=nil"), 1);
}

#ifdef AMPERSAND_MODULE_INPUT

using ampersand::input::InputManager;
using ampersand::input::KeyCode;
using ampersand::input::KeyPressed;

TEST_F(ScriptingSystemTest, InputAxisIsReadable) {
    auto& manager = _world.addResource<InputManager>();
    manager.setAxis("Horizontal",
                    {.positive = KeyCode::D, .negative = KeyCode::A});
    manager.beginFrame();
    manager.apply(KeyPressed{KeyCode::D});
    spawn({
        script("axis.lua", R"(
        local S = {}
        function S:update() error("axis=" .. input.getAxis("Horizontal")) end
        return S
    )"),
    });

    EXPECT_EQ(count(frame(), "axis=1"), 1);
}

TEST_F(ScriptingSystemTest, UnknownAxisIsAScriptError) {
    _world.addResource<InputManager>();
    spawn({
        script("unknown_axis.lua", R"(
        local S = {}
        function S:update() input.getAxis("Unknown") end
        return S
    )"),
    });

    EXPECT_EQ(frame().size(), 1U);
}

TEST_F(ScriptingSystemTest, InputButtonIsReadable) {
    auto& manager = _world.addResource<InputManager>();
    manager.setButton("Fire", {KeyCode::Space});
    manager.beginFrame();
    manager.apply(KeyPressed{KeyCode::Space});
    spawn({
        script("button.lua", R"(
        local S = {}
        function S:update()
            error("held=" .. tostring(input.getButton("Fire")) ..
                  " down=" .. tostring(input.getButtonDown("Fire")) ..
                  " up=" .. tostring(input.getButtonUp("Fire")))
        end
        return S
    )"),
    });

    EXPECT_EQ(count(frame(), "held=true down=true up=false"), 1);
}

#endif

TEST_F(ScriptingSystemTest, RectsIntersect) {
    spawn({
        script("rect.lua", R"(
        local S = {}
        function S:update()
            local box = Rect.new(0, 0, 10, 10)
            error("near=" .. tostring(box:intersects(Rect.new(5, 5, 10, 10)))
                  .. " far=" .. tostring(box:intersects(Rect.new(50, 50, 1, 1))))
        end
        return S
    )"),
    });

    EXPECT_EQ(count(frame(), "near=true far=false"), 1);
}

#ifdef AMPERSAND_MODULE_PHYSICS

using ampersand::physics::BoxCollider;

TEST_F(ScriptingSystemTest, BoxColliderIsReadable) {
    const Entity entity = spawn({
        script("collider.lua", R"(
        local S = {}
        function S:update()
            error("width=" .. getBoxCollider(self.entity).bounds.width
                  .. " has=" .. tostring(hasBoxCollider(self.entity)))
        end
        return S
    )"),
    });
    _world.add(entity, BoxCollider{.bounds = {0.0F, 0.0F, 16.0F, 8.0F}});

    EXPECT_EQ(count(frame(), "width=16"), 1);
}

TEST_F(ScriptingSystemTest, GetBoundsGivesTheBoundsInTheWorld) {
    const Entity entity = spawn({
        script("bounds.lua", R"(
        local S = {}
        function S:update()
            local bounds = getBounds(self.entity)
            error("x=" .. bounds.x .. " y=" .. bounds.y)
        end
        return S
    )"),
    });
    Transform transform;
    transform.position = {100.0F, 50.0F, 0.0F};
    _world.add(entity, transform);
    _world.add(entity, BoxCollider{.bounds = {2.0F, 3.0F, 10.0F, 10.0F}});

    EXPECT_EQ(count(frame(), "x=102.0 y=53.0"), 1);
}

TEST_F(ScriptingSystemTest, ScriptFindsTheCollidersItTouches) {
    const Entity missile = spawn({
        script("missile.lua", R"(
        local S = {}
        function S:update()
            local bounds = getBounds(self.entity)
            for _, other in ipairs(getColliders()) do
                if other.index ~= self.entity.index
                    and bounds:intersects(getBounds(other)) then
                    error("hit=" .. other.index)
                end
            end
        end
        return S
    )"),
    });
    const Entity near = _world.create();
    const Entity far = _world.create();
    const BoxCollider box{.bounds = {0.0F, 0.0F, 10.0F, 10.0F}};
    Transform farTransform;
    farTransform.position = {500.0F, 500.0F, 0.0F};
    for (const Entity entity : {missile, near}) {
        _world.add(entity, Transform{});
        _world.add(entity, box);
    }
    _world.add(far, farTransform);
    _world.add(far, box);

    const auto messages = frame();
    ASSERT_EQ(messages.size(), 1U);
    EXPECT_NE(messages.front().find("hit=" + std::to_string(near.index)),
              std::string::npos);
}

TEST_F(ScriptingSystemTest, GetBoundsWithoutABoxColliderIsAScriptError) {
    const Entity entity = spawn({
        script("no_collider.lua", R"(
        local S = {}
        function S:update() getBounds(self.entity) end
        return S
    )"),
    });
    _world.add(entity, Transform{});

    EXPECT_EQ(frame().size(), 1U);
}

#endif

#ifdef AMPERSAND_MODULE_RENDER

using ampersand::render::Animation;
using ampersand::render::AnimationStateMachine;

/** @brief State machine with the states "idle" and "attack". */
AnimationStateMachine stateMachine() {
    const Animation clip{.frames = {{0, 0, 16, 16}}, .frameDuration = 0.5F};
    return {{{"idle", clip}, {"attack", clip}}, "idle"};
}

TEST_F(ScriptingSystemTest, AnimationParamsAreSet) {
    const Entity entity = spawn({
        script("params.lua", R"(
        local S = {}
        function S:update()
            animation.setBool(self.entity, "moving", true)
            animation.setFloat(self.entity, "speed", 2.5)
        end
        return S
    )"),
    });
    _world.add(entity, stateMachine());

    EXPECT_TRUE(frame().empty());
    const auto& params = _world.get<AnimationStateMachine>(entity).params;
    EXPECT_TRUE(params.bools.at("moving"));
    EXPECT_FLOAT_EQ(params.floats.at("speed"), 2.5F);
}

TEST_F(ScriptingSystemTest, AnimationPlayOnceAndState) {
    const Entity entity = spawn({
        script("attack.lua", R"(
        local S = {}
        function S:update()
            animation.playOnce(self.entity, "attack")
            error("playing=" .. tostring(animation.isPlayingAction(self.entity))
                  .. " state=" .. animation.getState(self.entity))
        end
        return S
    )"),
    });
    _world.add(entity, stateMachine());

    EXPECT_EQ(count(frame(), "playing=true state=idle"), 1);
}

TEST_F(ScriptingSystemTest, AnimationWithoutAStateMachineIsAScriptError) {
    spawn({
        script("no_machine.lua", R"(
        local S = {}
        function S:update() animation.setBool(self.entity, "moving", true) end
        return S
    )"),
    });

    EXPECT_EQ(frame().size(), 1U);
}

#endif

#ifdef AMPERSAND_MODULE_AUDIO

using ampersand::audio::AudioSource;

TEST_F(ScriptingSystemTest, ScriptPlaysItsAudioSource) {
    const Entity entity = spawn({
        script("laser.lua", R"(
        local S = {}
        function S:start()
            local source = getAudioSource(self.entity)
            source.volume = 40
            source.loop = true
            source.playing = true
        end
        return S
    )"),
    });
    _world.add(entity, AudioSource{});

    EXPECT_TRUE(frame().empty());
    const auto& source = _world.get<AudioSource>(entity);
    EXPECT_FLOAT_EQ(source.volume, 40.0F);
    EXPECT_TRUE(source.loop);
    EXPECT_TRUE(source.playing);
}

TEST_F(ScriptingSystemTest, HasAudioSourceTellsIfThereIsOne) {
    const auto path = script("has_source.lua", R"(
        local S = {}
        function S:update()
            error("has=" .. tostring(hasAudioSource(self.entity)))
        end
        return S
    )");
    const Entity withSource = spawn({path});
    _world.add(withSource, AudioSource{});
    spawn({path});

    const auto messages = frame();
    EXPECT_EQ(count(messages, "has=true"), 1);
    EXPECT_EQ(count(messages, "has=false"), 1);
}

TEST_F(ScriptingSystemTest, GetAudioSourceWithoutOneIsAScriptError) {
    spawn({
        script("no_source.lua", R"(
        local S = {}
        function S:start() getAudioSource(self.entity).playing = true end
        return S
    )"),
    });

    EXPECT_EQ(count(frame(), "Component not found"), 1);
}

#endif
