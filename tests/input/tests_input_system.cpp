#include <gtest/gtest.h>

#include <ampersand/core/ecs/ECSExceptions.hpp>
#include <ampersand/core/ecs/World.hpp>
#include <ampersand/input/Event.hpp>
#include <ampersand/input/resources/InputManager.hpp>
#include <ampersand/input/systems/InputSystem.hpp>
#include <deque>
#include <optional>
#include <utility>

using ampersand::core::ecs::ResourceNotFound;
using ampersand::core::ecs::World;
using ampersand::input::Event;
using ampersand::input::InputManager;
using ampersand::input::InputSystem;
using ampersand::input::JoystickAxis;
using ampersand::input::JoystickButton;
using ampersand::input::JoystickButtonPressed;
using ampersand::input::JoystickMoved;
using ampersand::input::KeyCode;
using ampersand::input::KeyPressed;
using ampersand::input::KeyReleased;
using ampersand::input::MouseButton;
using ampersand::input::MouseMoved;
using ampersand::input::MousePressed;
using ampersand::input::MouseReleased;

/** @brief Events waiting to be polled, in place of a window. */
struct PendingEvents {
    std::deque<Event> events;
};

/** @brief Poll function of the tests: gives the PendingEvents one by one. */
std::optional<Event> pollPending(World& world) {
    auto& pending = world.getResource<PendingEvents>().events;
    if (pending.empty()) {
        return std::nullopt;
    }
    const Event event = pending.front();
    pending.pop_front();
    return event;
}

/** @brief Runs one frame of the InputSystem on these events. */
void runFrame(World& world, std::deque<Event> events) {
    world.getResource<PendingEvents>().events = std::move(events);
    InputSystem(pollPending).update(world);
}

TEST(InputSystem, ThrowsWithoutInputManager) {
    World world;
    world.addResource<PendingEvents>();

    EXPECT_THROW(runFrame(world, {}), ResourceNotFound);
}

TEST(InputSystem, HoldsAPressedKey) {
    World world;
    const auto& manager = world.addResource<InputManager>();
    world.addResource<PendingEvents>();

    runFrame(world, {KeyPressed{KeyCode::Space}});

    EXPECT_TRUE(manager.keysPressed().contains(KeyCode::Space));
    EXPECT_TRUE(manager.keysDown().contains(KeyCode::Space));
}

TEST(InputSystem, IgnoresTheRepeatOfAHeldKey) {
    World world;
    const auto& manager = world.addResource<InputManager>();
    world.addResource<PendingEvents>();

    runFrame(world, {KeyPressed{KeyCode::A}, KeyPressed{KeyCode::A}});
    EXPECT_TRUE(manager.keysPressed().contains(KeyCode::A));

    runFrame(world, {KeyPressed{KeyCode::A}});
    EXPECT_TRUE(manager.keysPressed().empty());
    EXPECT_TRUE(manager.keysDown().contains(KeyCode::A));
}

TEST(InputSystem, ReleasesAKey) {
    World world;
    const auto& manager = world.addResource<InputManager>();
    world.addResource<PendingEvents>();
    runFrame(world, {KeyPressed{KeyCode::LeftArrow}});

    runFrame(world, {KeyReleased{KeyCode::LeftArrow}});

    EXPECT_TRUE(manager.keysReleased().contains(KeyCode::LeftArrow));
    EXPECT_TRUE(manager.keysDown().empty());
}

TEST(InputSystem, SeesAKeyPressedAndReleasedInTheSameFrame) {
    World world;
    const auto& manager = world.addResource<InputManager>();
    const auto& pending = world.addResource<PendingEvents>().events;

    runFrame(world, {
                        KeyPressed{KeyCode::Z},
                        KeyPressed{KeyCode::Q},
                        KeyReleased{KeyCode::Z},
                    });

    EXPECT_TRUE(pending.empty());
    EXPECT_TRUE(manager.keysPressed().contains(KeyCode::Z));
    EXPECT_TRUE(manager.keysPressed().contains(KeyCode::Q));
    EXPECT_TRUE(manager.keysReleased().contains(KeyCode::Z));
    EXPECT_FALSE(manager.keysReleased().contains(KeyCode::Q));
    EXPECT_EQ(manager.keysDown().size(), 1U);
    EXPECT_TRUE(manager.keysDown().contains(KeyCode::Q));
}

TEST(InputSystem, EmptiesThePressedAndReleasedEachFrame) {
    World world;
    const auto& manager = world.addResource<InputManager>();
    world.addResource<PendingEvents>();
    runFrame(world, {
                        KeyPressed{KeyCode::UpArrow},
                        KeyPressed{KeyCode::Tab},
                        KeyReleased{KeyCode::Tab},
                        MouseMoved{{4, 2}},
                    });

    runFrame(world, {});

    EXPECT_TRUE(manager.keysPressed().empty());
    EXPECT_TRUE(manager.keysReleased().empty());
    EXPECT_TRUE(manager.keysDown().contains(KeyCode::UpArrow));
    EXPECT_EQ(manager.mouse().x, 4);
    EXPECT_EQ(manager.mouse().y, 2);
}

TEST(InputSystem, HoldsAPressedButtonAndMovesTheMouse) {
    World world;
    const auto& manager = world.addResource<InputManager>();
    world.addResource<PendingEvents>();

    runFrame(world,
             {MousePressed{.button = MouseButton::Left, .position = {10, 20}}});

    EXPECT_TRUE(manager.buttonsPressed().contains(MouseButton::Left));
    EXPECT_TRUE(manager.buttonsDown().contains(MouseButton::Left));
    EXPECT_EQ(manager.mouse().x, 10);
    EXPECT_EQ(manager.mouse().y, 20);
}

TEST(InputSystem, IgnoresThePressOfAHeldButton) {
    World world;
    const auto& manager = world.addResource<InputManager>();
    world.addResource<PendingEvents>();

    runFrame(world,
             {
                 MousePressed{.button = MouseButton::Right, .position = {1, 1}},
                 MousePressed{.button = MouseButton::Right, .position = {3, 3}},
             });

    EXPECT_EQ(manager.buttonsPressed().size(), 1U);
    EXPECT_EQ(manager.buttonsDown().size(), 1U);
    EXPECT_EQ(manager.mouse().x, 3);
}

TEST(InputSystem, ReleasesAButtonAndMovesTheMouse) {
    World world;
    const auto& manager = world.addResource<InputManager>();
    world.addResource<PendingEvents>();
    runFrame(world,
             {MousePressed{.button = MouseButton::Middle, .position = {1, 2}}});

    runFrame(world, {
                        MouseReleased{
                            .button = MouseButton::Middle,
                            .position = {30, 40},
                        },
                    });

    EXPECT_TRUE(manager.buttonsReleased().contains(MouseButton::Middle));
    EXPECT_TRUE(manager.buttonsDown().empty());
    EXPECT_EQ(manager.mouse().x, 30);
    EXPECT_EQ(manager.mouse().y, 40);
}

TEST(InputSystem, FollowsTheMouse) {
    World world;
    const auto& manager = world.addResource<InputManager>();
    world.addResource<PendingEvents>();

    runFrame(world, {MouseMoved{{1, 1}}, MouseMoved{{5, 6}}});

    EXPECT_EQ(manager.mouse().x, 5);
    EXPECT_EQ(manager.mouse().y, 6);
}

TEST(InputSystem, AppliesTheJoystickEvents) {
    World world;
    const auto& manager = world.addResource<InputManager>();
    world.addResource<PendingEvents>();

    runFrame(world, {
                        JoystickButtonPressed{
                            .joystick = 0,
                            .button = JoystickButton::Button2,
                        },
                        JoystickMoved{
                            .joystick = 0,
                            .axis = JoystickAxis::X,
                            .position = 0.5F,
                        },
                    });
    EXPECT_TRUE(manager.getJoystickButtonDown(JoystickButton::Button2));
    EXPECT_FLOAT_EQ(manager.getJoystickAxis(JoystickAxis::X), 0.5F);

    runFrame(world, {});
    EXPECT_FALSE(manager.getJoystickButtonDown(JoystickButton::Button2));
    EXPECT_TRUE(manager.getJoystickButton(JoystickButton::Button2));
    EXPECT_FLOAT_EQ(manager.getJoystickAxis(JoystickAxis::X), 0.5F);
}
