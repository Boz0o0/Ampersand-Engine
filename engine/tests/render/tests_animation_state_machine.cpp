#include <gtest/gtest.h>

#include <ampersand/core/ecs/World.hpp>
#include <ampersand/core/resources/Clock.hpp>
#include <ampersand/render/components/Animation.hpp>
#include <ampersand/render/components/AnimationStateMachine.hpp>
#include <ampersand/render/components/Sprite.hpp>
#include <ampersand/render/systems/AnimationSystem.hpp>
#include <cstdint>
#include <stdexcept>
#include <utility>

namespace {

using ampersand::core::Clock;
using ampersand::core::ecs::Entity;
using ampersand::core::ecs::World;
using ampersand::render::Animation;
using ampersand::render::AnimationParams;
using ampersand::render::AnimationStateMachine;
using ampersand::render::AnimationSystem;
using ampersand::render::AnimationTransition;
using ampersand::render::Sprite;

/** @brief Two 16x16 frames of half a second on row `row` (y = row * 16). */
Animation clip(std::int32_t row, bool loop = true) {
    return {
        .frames = {{0, row * 16, 16, 16}, {16, row * 16, 16, 16}},
        .frameDuration = 0.5F,
        .loop = loop,
    };
}

/** @brief Transition from `source` to `target` that always fires. */
AnimationTransition always(const char* source, const char* target) {
    return {
        .from = source,
        .to = target,
        .condition = [](const AnimationParams&) { return true; },
    };
}

/** @brief idle (row 0), run (row 1), jump (row 2), attack (row 3), hurt
 * (row 4); idle -> run when "speed" > 0, run -> idle when it is 0, any ->
 * jump when "jumping" is true. */
AnimationStateMachine player() {
    AnimationStateMachine machine{
        {
            {"idle", clip(0)},
            {"run", clip(1)},
            {"jump", clip(2)},
            {"attack", clip(3)},
            {"hurt", clip(4)},
        },
        "idle",
    };
    machine.transitions = {
        {
            .from = "*",
            .to = "jump",
            .condition =
                [](const AnimationParams& params) {
                    return params.bools.contains("jumping") &&
                           params.bools.at("jumping");
                },
        },
        {
            .from = "idle",
            .to = "run",
            .condition =
                [](const AnimationParams& params) {
                    return params.floats.contains("speed") &&
                           params.floats.at("speed") > 0.0F;
                },
        },
        {
            .from = "run",
            .to = "idle",
            .condition =
                [](const AnimationParams& params) {
                    return !params.floats.contains("speed") ||
                           params.floats.at("speed") == 0.0F;
                },
        },
    };
    return machine;
}

Entity createPlayer(World& world, AnimationStateMachine machine = player()) {
    const Entity entity = world.create();
    world.add(entity, Sprite{.texture = 1});
    world.add(entity, std::move(machine));
    return entity;
}

void step(World& world, float deltaTime) {
    world.getResource<Clock>().dt = deltaTime;
    AnimationSystem{}.update(world);
}

AnimationStateMachine& machineOf(World& world, Entity entity) {
    return world.get<AnimationStateMachine>(entity);
}

/** @brief Row of the clip shown on the Sprite. */
std::int32_t shownRow(World& world, Entity entity) {
    return world.get<Sprite>(entity).source.y / 16;
}

std::int32_t shownFrame(World& world, Entity entity) {
    return world.get<Sprite>(entity).source.x / 16;
}

}  // namespace

TEST(AnimationStateMachine, AddsAnAnimationAndShowsTheCurrentState) {
    World world;
    world.addResource<Clock>();
    const Entity hero = createPlayer(world);

    step(world, 0.0F);

    EXPECT_TRUE(world.has<Animation>(hero));
    EXPECT_EQ(shownRow(world, hero), 0);
}

TEST(AnimationStateMachine, FloatParamTriggersATransition) {
    World world;
    world.addResource<Clock>();
    const Entity hero = createPlayer(world);
    step(world, 0.0F);

    machineOf(world, hero).params.floats["speed"] = 3.0F;
    step(world, 0.0F);

    EXPECT_EQ(machineOf(world, hero).current(), "run");
    EXPECT_EQ(shownRow(world, hero), 1);

    machineOf(world, hero).params.floats["speed"] = 0.0F;
    step(world, 0.0F);

    EXPECT_EQ(machineOf(world, hero).current(), "idle");
}

TEST(AnimationStateMachine, AnyStateTransitionFiresFromEveryState) {
    World world;
    world.addResource<Clock>();
    const Entity hero = createPlayer(world);
    machineOf(world, hero).setCurrent("run");
    machineOf(world, hero).params.floats["speed"] = 3.0F;
    machineOf(world, hero).params.bools["jumping"] = true;

    step(world, 0.0F);

    EXPECT_EQ(machineOf(world, hero).current(), "jump");
    EXPECT_EQ(shownRow(world, hero), 2);
}

TEST(AnimationStateMachine, FirstMatchWinsAndOnlyOneTransitionPerUpdate) {
    World world;
    world.addResource<Clock>();
    const Entity hero = createPlayer(world);
    machineOf(world, hero).params.floats["speed"] = 3.0F;
    machineOf(world, hero).params.bools["jumping"] = true;

    step(world, 0.0F);
    EXPECT_EQ(machineOf(world, hero).current(), "jump");

    machineOf(world, hero).setCurrent("idle");
    machineOf(world, hero).params.bools["jumping"] = false;
    machineOf(world, hero).params.floats["speed"] = 0.0F;
    machineOf(world, hero).transitions.push_back(always("idle", "run"));
    step(world, 0.0F);

    EXPECT_EQ(machineOf(world, hero).current(), "run");
}

TEST(AnimationStateMachine, SwitchingStateRestartsTheClip) {
    World world;
    world.addResource<Clock>();
    const Entity hero = createPlayer(world);
    step(world, 0.5F);
    ASSERT_EQ(shownFrame(world, hero), 1);

    machineOf(world, hero).params.floats["speed"] = 3.0F;
    step(world, 0.0F);

    EXPECT_EQ(shownRow(world, hero), 1);
    EXPECT_EQ(shownFrame(world, hero), 0);
}

TEST(AnimationStateMachine, ActionPlaysOnceThenGivesTheBaseStateBack) {
    World world;
    world.addResource<Clock>();
    const Entity hero = createPlayer(world);
    step(world, 0.0F);

    machineOf(world, hero).playOnce("attack");
    step(world, 0.0F);
    EXPECT_TRUE(machineOf(world, hero).isPlayingAction());
    EXPECT_EQ(shownRow(world, hero), 3);
    EXPECT_FALSE(world.get<Animation>(hero).loop);

    step(world, 1.0F);
    EXPECT_EQ(shownFrame(world, hero), 1);
    EXPECT_TRUE(machineOf(world, hero).isPlayingAction());

    step(world, 0.0F);
    EXPECT_FALSE(machineOf(world, hero).isPlayingAction());
    EXPECT_EQ(shownRow(world, hero), 0);
    EXPECT_TRUE(world.get<Animation>(hero).loop);
}

TEST(AnimationStateMachine, TransitionsArePausedDuringAnAction) {
    World world;
    world.addResource<Clock>();
    const Entity hero = createPlayer(world);
    step(world, 0.0F);
    machineOf(world, hero).playOnce("attack");
    step(world, 0.0F);

    machineOf(world, hero).params.floats["speed"] = 3.0F;
    step(world, 0.5F);
    EXPECT_EQ(machineOf(world, hero).current(), "idle");
    EXPECT_EQ(shownRow(world, hero), 3);

    step(world, 0.5F);
    step(world, 0.0F);
    EXPECT_EQ(machineOf(world, hero).current(), "run");
    EXPECT_EQ(shownRow(world, hero), 1);
}

TEST(AnimationStateMachine, NewActionReplacesThePlayingOne) {
    World world;
    world.addResource<Clock>();
    const Entity hero = createPlayer(world);
    machineOf(world, hero).playOnce("attack");
    step(world, 0.5F);
    ASSERT_EQ(shownFrame(world, hero), 1);

    machineOf(world, hero).playOnce("hurt");
    step(world, 0.0F);

    EXPECT_TRUE(machineOf(world, hero).isPlayingAction());
    EXPECT_EQ(shownRow(world, hero), 4);
    EXPECT_EQ(shownFrame(world, hero), 0);
}

TEST(AnimationStateMachine, SameActionAgainRestartsIt) {
    World world;
    world.addResource<Clock>();
    const Entity hero = createPlayer(world);
    machineOf(world, hero).playOnce("attack");
    step(world, 0.5F);
    ASSERT_EQ(shownFrame(world, hero), 1);

    machineOf(world, hero).playOnce("attack");
    step(world, 0.0F);

    EXPECT_EQ(shownFrame(world, hero), 0);
}

TEST(AnimationStateMachine, PlayOnceThrowsOnUnknownState) {
    AnimationStateMachine machine = player();

    EXPECT_THROW(machine.playOnce("dance"), std::invalid_argument);
    EXPECT_FALSE(machine.isPlayingAction());
}

TEST(AnimationStateMachine, ConstructorThrowsOnUnknownInitialState) {
    EXPECT_THROW(AnimationStateMachine({{"idle", clip(0)}}, "dance"),
                 std::invalid_argument);
}

TEST(AnimationStateMachine, SetCurrentThrowsOnUnknownState) {
    AnimationStateMachine machine = player();

    EXPECT_THROW(machine.setCurrent("dance"), std::invalid_argument);
    EXPECT_EQ(machine.current(), "idle");
}

TEST(AnimationStateMachine, SetCurrentSwitchesTheClip) {
    World world;
    world.addResource<Clock>();
    const Entity hero = createPlayer(world);
    step(world, 0.0F);

    machineOf(world, hero).setCurrent("hurt");
    step(world, 0.0F);

    EXPECT_EQ(shownRow(world, hero), 4);
}

TEST(AnimationStateMachine, ThrowsWhenTheCurrentStateIsRemoved) {
    World world;
    world.addResource<Clock>();
    const Entity hero = createPlayer(world);
    machineOf(world, hero).states.erase("idle");

    EXPECT_THROW(step(world, 0.0F), std::invalid_argument);
}

TEST(AnimationStateMachine, ThrowsOnTransitionToUnknownState) {
    World world;
    world.addResource<Clock>();
    AnimationStateMachine machine = player();
    machine.transitions = {always("idle", "dance")};
    const Entity hero = createPlayer(world, std::move(machine));

    EXPECT_THROW(step(world, 0.0F), std::invalid_argument);
    EXPECT_EQ(machineOf(world, hero).current(), "idle");
}

TEST(AnimationStateMachine, TransitionToTheCurrentStateBlocksTheNextOnes) {
    World world;
    world.addResource<Clock>();
    AnimationStateMachine machine = player();
    machine.transitions.push_back(always("jump", "idle"));
    const Entity hero = createPlayer(world, std::move(machine));
    machineOf(world, hero).params.bools["jumping"] = true;

    step(world, 0.0F);
    step(world, 0.0F);

    EXPECT_EQ(machineOf(world, hero).current(), "jump");
}

TEST(AnimationStateMachine, ActionOnTheBaseStateLoopsAgainAfterIt) {
    World world;
    world.addResource<Clock>();
    const Entity hero = createPlayer(world);
    step(world, 0.0F);

    machineOf(world, hero).playOnce("idle");
    step(world, 0.0F);
    EXPECT_FALSE(world.get<Animation>(hero).loop);

    step(world, 1.0F);
    step(world, 0.0F);

    EXPECT_FALSE(machineOf(world, hero).isPlayingAction());
    EXPECT_TRUE(world.get<Animation>(hero).loop);
}

TEST(AnimationStateMachine, RemovedAnimationIsAddedAndLoadedAgain) {
    World world;
    world.addResource<Clock>();
    const Entity hero = createPlayer(world);
    step(world, 0.0F);

    world.remove<Animation>(hero);
    step(world, 0.0F);

    EXPECT_EQ(world.get<Animation>(hero).frames.size(), 2U);
    EXPECT_EQ(shownRow(world, hero), 0);
}

TEST(AnimationStateMachine, ReloadLoadsTheEditedClip) {
    World world;
    world.addResource<Clock>();
    const Entity hero = createPlayer(world);
    step(world, 0.0F);

    machineOf(world, hero).states["idle"] = clip(5);
    step(world, 0.0F);
    EXPECT_EQ(shownRow(world, hero), 0);

    machineOf(world, hero).reload();
    step(world, 0.0F);
    EXPECT_EQ(shownRow(world, hero), 5);
}
