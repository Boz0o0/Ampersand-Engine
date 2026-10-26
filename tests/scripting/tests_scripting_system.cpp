#include <gtest/gtest.h>

#include <algorithm>
#include <ampersand/core/ecs/ECSExceptions.hpp>
#include <ampersand/core/ecs/EventQueue.hpp>
#include <ampersand/core/ecs/World.hpp>
#include <ampersand/core/resources/Clock.hpp>
#include <ampersand/scripting/components/Scripts.hpp>
#include <ampersand/scripting/systems/ScriptingSystem.hpp>
#include <filesystem>
#include <fstream>
#include <string>
#include <vector>

using ampersand::core::Clock;
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
 * Lua cannot reach the engine yet, so the scripts tell what happened by raising
 * an error: its message is read back from the EventQueue<ScriptError>.
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
