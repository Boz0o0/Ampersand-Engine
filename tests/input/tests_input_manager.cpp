#include <gtest/gtest.h>

#include <ampersand/input/Event.hpp>
#include <ampersand/input/resources/InputManager.hpp>
#include <initializer_list>
#include <stdexcept>

using ampersand::input::Axis;
using ampersand::input::Binding;
using ampersand::input::Event;
using ampersand::input::FocusLost;
using ampersand::input::InputManager;
using ampersand::input::JoystickAxis;
using ampersand::input::JoystickButton;
using ampersand::input::JoystickButtonPressed;
using ampersand::input::JoystickButtonReleased;
using ampersand::input::JoystickMoved;
using ampersand::input::KeyCode;
using ampersand::input::KeyPressed;
using ampersand::input::KeyReleased;
using ampersand::input::MouseButton;
using ampersand::input::MousePressed;
using ampersand::input::MouseReleased;

/** @brief Starts a new frame of the manager and applies these events. */
void frame(InputManager& manager, std::initializer_list<Event> events) {
    manager.beginFrame();
    for (const Event& event : events) {
        manager.apply(event);
    }
}

/** @brief Event moving the axis of the controller. */
JoystickMoved moved(unsigned int joystick, JoystickAxis axis, float position) {
    return {.joystick = joystick, .axis = axis, .position = position};
}

TEST(InputManager, ReadsKeysWithTheGetFunctions) {
    InputManager manager;

    frame(manager, {KeyPressed{KeyCode::W}});
    EXPECT_TRUE(manager.getKey(KeyCode::W));
    EXPECT_TRUE(manager.getKeyDown(KeyCode::W));
    EXPECT_FALSE(manager.getKeyUp(KeyCode::W));

    frame(manager, {});
    EXPECT_TRUE(manager.getKey(KeyCode::W));
    EXPECT_FALSE(manager.getKeyDown(KeyCode::W));

    frame(manager, {KeyReleased{KeyCode::W}});
    EXPECT_FALSE(manager.getKey(KeyCode::W));
    EXPECT_TRUE(manager.getKeyUp(KeyCode::W));
}

TEST(InputManager, ReadsMouseButtonsWithTheGetFunctions) {
    InputManager manager;

    frame(manager,
          {MousePressed{.button = MouseButton::Left, .position = {0, 0}}});
    EXPECT_TRUE(manager.getMouseButton(MouseButton::Left));
    EXPECT_TRUE(manager.getMouseButtonDown(MouseButton::Left));
    EXPECT_FALSE(manager.getMouseButton(MouseButton::Right));

    frame(manager,
          {MouseReleased{.button = MouseButton::Left, .position = {0, 0}}});
    EXPECT_FALSE(manager.getMouseButton(MouseButton::Left));
    EXPECT_TRUE(manager.getMouseButtonUp(MouseButton::Left));
}

TEST(InputManager, IgnoresNoneKeysAndButtons) {
    InputManager manager;

    frame(manager,
          {
              KeyPressed{KeyCode::None},
              MousePressed{.button = MouseButton::None, .position = {7, 8}},
              JoystickButtonPressed{
                  .joystick = 0,
                  .button = JoystickButton::None,
              },
          });

    EXPECT_TRUE(manager.keysDown().empty());
    EXPECT_TRUE(manager.buttonsDown().empty());
    EXPECT_TRUE(manager.joystickButtonsPressed().empty());
    EXPECT_EQ(manager.mouse().x, 7);

    frame(manager,
          {
              KeyReleased{KeyCode::None},
              MouseReleased{.button = MouseButton::None, .position = {0, 0}},
              JoystickButtonReleased{
                  .joystick = 0,
                  .button = JoystickButton::None,
              },
          });

    EXPECT_TRUE(manager.keysReleased().empty());
    EXPECT_TRUE(manager.buttonsReleased().empty());
    EXPECT_TRUE(manager.joystickButtonsReleased().empty());
}

TEST(InputManager, FocusLostReleasesKeysAndMouseButtons) {
    InputManager manager;
    frame(manager,
          {
              KeyPressed{KeyCode::A},
              MousePressed{.button = MouseButton::Right, .position = {0, 0}},
              JoystickButtonPressed{
                  .joystick = 0,
                  .button = JoystickButton::Button0,
              },
              moved(0, JoystickAxis::X, 0.5F),
          });

    frame(manager, {FocusLost{}});

    EXPECT_FALSE(manager.getKey(KeyCode::A));
    EXPECT_TRUE(manager.getKeyUp(KeyCode::A));
    EXPECT_FALSE(manager.getMouseButton(MouseButton::Right));
    EXPECT_TRUE(manager.getMouseButtonUp(MouseButton::Right));
    EXPECT_TRUE(manager.getJoystickButton(JoystickButton::Button0));
    EXPECT_FLOAT_EQ(manager.getJoystickAxis(JoystickAxis::X), 0.5F);
}

TEST(InputManager, HoldsAndReleasesAJoystickButton) {
    InputManager manager;

    frame(manager, {
                       JoystickButtonPressed{
                           .joystick = 0,
                           .button = JoystickButton::Button3,
                       },
                   });
    EXPECT_TRUE(manager.getJoystickButton(JoystickButton::Button3));
    EXPECT_TRUE(manager.getJoystickButtonDown(JoystickButton::Button3));
    EXPECT_FALSE(manager.getJoystickButtonUp(JoystickButton::Button3));
    EXPECT_FALSE(manager.getJoystickButton(JoystickButton::Button2));

    frame(manager, {});
    EXPECT_TRUE(manager.getJoystickButton(JoystickButton::Button3));
    EXPECT_FALSE(manager.getJoystickButtonDown(JoystickButton::Button3));

    frame(manager, {
                       JoystickButtonReleased{
                           .joystick = 0,
                           .button = JoystickButton::Button3,
                       },
                   });
    EXPECT_FALSE(manager.getJoystickButton(JoystickButton::Button3));
    EXPECT_TRUE(manager.getJoystickButtonUp(JoystickButton::Button3));
}

TEST(InputManager, IgnoresThePressOfAHeldJoystickButton) {
    InputManager manager;
    frame(manager, {
                       JoystickButtonPressed{
                           .joystick = 1,
                           .button = JoystickButton::Button0,
                       },
                   });

    frame(manager, {
                       JoystickButtonPressed{
                           .joystick = 1,
                           .button = JoystickButton::Button0,
                       },
                   });

    EXPECT_FALSE(manager.getJoystickButtonDown(JoystickButton::Button0));
    EXPECT_TRUE(manager.getJoystickButton(JoystickButton::Button0));
}

TEST(InputManager, KeepsTheJoystickButtonsPerController) {
    InputManager manager;
    frame(manager, {
                       JoystickButtonPressed{
                           .joystick = 0,
                           .button = JoystickButton::Button1,
                       },
                       JoystickButtonPressed{
                           .joystick = 1,
                           .button = JoystickButton::Button1,
                       },
                   });

    frame(manager, {
                       JoystickButtonReleased{
                           .joystick = 0,
                           .button = JoystickButton::Button1,
                       },
                   });

    EXPECT_TRUE(manager.getJoystickButton(JoystickButton::Button1));
    EXPECT_TRUE(manager.getJoystickButtonUp(JoystickButton::Button1));
    EXPECT_FALSE(
        manager.joystickButtonsDown().at(0).contains(JoystickButton::Button1));
    EXPECT_TRUE(
        manager.joystickButtonsDown().at(1).contains(JoystickButton::Button1));
}

TEST(InputManager, ReadsAJoystickAxis) {
    InputManager manager;
    EXPECT_FLOAT_EQ(manager.getJoystickAxis(JoystickAxis::Y), 0.0F);

    frame(manager, {moved(0, JoystickAxis::Y, -0.75F)});
    EXPECT_FLOAT_EQ(manager.getJoystickAxis(JoystickAxis::Y), -0.75F);
    EXPECT_FLOAT_EQ(manager.getJoystickAxis(JoystickAxis::X), 0.0F);

    frame(manager, {});
    EXPECT_FLOAT_EQ(manager.getJoystickAxis(JoystickAxis::Y), -0.75F);
    EXPECT_FLOAT_EQ(manager.joystickAxes().at(0).at(JoystickAxis::Y), -0.75F);
}

TEST(InputManager, ReadsTheJoystickAxisFarthestFromZero) {
    InputManager manager;

    frame(manager, {
                       moved(0, JoystickAxis::X, 0.3F),
                       moved(1, JoystickAxis::X, -0.9F),
                       moved(2, JoystickAxis::X, 0.6F),
                   });

    EXPECT_FLOAT_EQ(manager.getJoystickAxis(JoystickAxis::X), -0.9F);
}

TEST(InputManager, ReadsEachKindOfBinding) {
    InputManager manager;
    frame(manager,
          {
              KeyPressed{KeyCode::E},
              MousePressed{.button = MouseButton::Middle, .position = {0, 0}},
              JoystickButtonPressed{
                  .joystick = 0,
                  .button = JoystickButton::Button5,
              },
          });

    EXPECT_TRUE(manager.getBinding(KeyCode::E));
    EXPECT_TRUE(manager.getBindingDown(MouseButton::Middle));
    EXPECT_TRUE(manager.getBindingDown(JoystickButton::Button5));
    EXPECT_FALSE(manager.getBinding(Binding{}));
    EXPECT_FALSE(manager.getBindingDown(Binding{}));

    frame(manager,
          {
              KeyReleased{KeyCode::E},
              MouseReleased{.button = MouseButton::Middle, .position = {0, 0}},
              JoystickButtonReleased{
                  .joystick = 0,
                  .button = JoystickButton::Button5,
              },
          });

    EXPECT_TRUE(manager.getBindingUp(KeyCode::E));
    EXPECT_TRUE(manager.getBindingUp(MouseButton::Middle));
    EXPECT_TRUE(manager.getBindingUp(JoystickButton::Button5));
    EXPECT_FALSE(manager.getBindingUp(Binding{}));
}

TEST(InputManager, ReadsAnAxisFromItsBindings) {
    InputManager manager;
    manager.setAxis("Horizontal",
                    {.positive = KeyCode::D, .negative = KeyCode::A});
    EXPECT_FLOAT_EQ(manager.getAxisRaw("Horizontal"), 0.0F);

    frame(manager, {KeyPressed{KeyCode::D}});
    EXPECT_FLOAT_EQ(manager.getAxisRaw("Horizontal"), 1.0F);

    frame(manager, {KeyPressed{KeyCode::A}});
    EXPECT_FLOAT_EQ(manager.getAxisRaw("Horizontal"), 0.0F);

    frame(manager, {KeyReleased{KeyCode::D}});
    EXPECT_FLOAT_EQ(manager.getAxisRaw("Horizontal"), -1.0F);
}

TEST(InputManager, ReadsAnAxisFromJoystickButtons) {
    InputManager manager;
    manager.setAxis("Zoom", {
                                .positive = JoystickButton::Button4,
                                .negative = JoystickButton::Button5,
                            });

    frame(manager, {
                       JoystickButtonPressed{
                           .joystick = 0,
                           .button = JoystickButton::Button5,
                       },
                   });

    EXPECT_FLOAT_EQ(manager.getAxisRaw("Zoom"), -1.0F);
}

TEST(InputManager, ReadsAnAxisFromTheJoystick) {
    InputManager manager;
    manager.setAxis("Horizontal", {.joystick = JoystickAxis::X});

    frame(manager, {moved(0, JoystickAxis::X, 0.5F)});

    EXPECT_FLOAT_EQ(manager.getAxisRaw("Horizontal"), 0.5F);
}

TEST(InputManager, ZeroesTheJoystickInsideTheDeadZone) {
    InputManager manager;
    manager.setAxis("Default", {.joystick = JoystickAxis::X});
    manager.setAxis("Wide", {.joystick = JoystickAxis::X, .dead = 0.5F});

    frame(manager, {moved(0, JoystickAxis::X, 0.19F)});
    EXPECT_FLOAT_EQ(manager.getAxisRaw("Default"), 0.0F);

    frame(manager, {moved(0, JoystickAxis::X, -0.4F)});
    EXPECT_FLOAT_EQ(manager.getAxisRaw("Default"), -0.4F);
    EXPECT_FLOAT_EQ(manager.getAxisRaw("Wide"), 0.0F);
}

TEST(InputManager, InvertsTheJoystickOfAnAxis) {
    InputManager manager;
    manager.setAxis("Vertical",
                    {.joystick = JoystickAxis::Y, .invertJoystick = true});

    frame(manager, {moved(0, JoystickAxis::Y, -0.8F)});

    EXPECT_FLOAT_EQ(manager.getAxisRaw("Vertical"), 0.8F);
}

TEST(InputManager, ReadsTheAxisSourceFarthestFromZero) {
    InputManager manager;
    manager.setAxis("Horizontal", {
                                      .positive = KeyCode::D,
                                      .negative = KeyCode::A,
                                      .joystick = JoystickAxis::X,
                                  });

    frame(manager, {moved(0, JoystickAxis::X, -0.6F)});
    EXPECT_FLOAT_EQ(manager.getAxisRaw("Horizontal"), -0.6F);

    frame(manager, {KeyPressed{KeyCode::D}});
    EXPECT_FLOAT_EQ(manager.getAxisRaw("Horizontal"), 1.0F);
}

TEST(InputManager, ReplacesAnAxisOfTheSameName) {
    InputManager manager;
    manager.setAxis("Horizontal",
                    {.positive = KeyCode::D, .negative = KeyCode::A});
    manager.setAxis("Horizontal", {
                                      .positive = KeyCode::RightArrow,
                                      .negative = KeyCode::LeftArrow,
                                  });

    frame(manager, {KeyPressed{KeyCode::D}, KeyPressed{KeyCode::LeftArrow}});

    EXPECT_FLOAT_EQ(manager.getAxisRaw("Horizontal"), -1.0F);
}

TEST(InputManager, ReadsAVirtualButtonFromAnyOfItsBindings) {
    InputManager manager;
    manager.setButton("Jump", {KeyCode::Space, JoystickButton::Button0});
    EXPECT_FALSE(manager.getButton("Jump"));

    frame(manager, {
                       JoystickButtonPressed{
                           .joystick = 0,
                           .button = JoystickButton::Button0,
                       },
                   });
    EXPECT_TRUE(manager.getButton("Jump"));
    EXPECT_TRUE(manager.getButtonDown("Jump"));

    frame(manager, {KeyPressed{KeyCode::Space}});
    EXPECT_TRUE(manager.getButtonDown("Jump"));

    frame(manager, {
                       JoystickButtonReleased{
                           .joystick = 0,
                           .button = JoystickButton::Button0,
                       },
                   });
    EXPECT_TRUE(manager.getButton("Jump"));
    EXPECT_TRUE(manager.getButtonUp("Jump"));
}

TEST(InputManager, ReadsAVirtualButtonFromTheMouse) {
    InputManager manager;
    manager.setButton("Fire", {MouseButton::Left});

    frame(manager,
          {MousePressed{.button = MouseButton::Left, .position = {0, 0}}});

    EXPECT_TRUE(manager.getButton("Fire"));
    EXPECT_TRUE(manager.getButtonDown("Fire"));
    EXPECT_FALSE(manager.getButtonUp("Fire"));
}

TEST(InputManager, ReplacesAVirtualButtonOfTheSameName) {
    InputManager manager;
    manager.setButton("Jump", {KeyCode::Space});
    manager.setButton("Jump", {KeyCode::W});

    frame(manager, {KeyPressed{KeyCode::Space}});

    EXPECT_FALSE(manager.getButton("Jump"));
}

TEST(InputManager, ThrowsOnAnUnknownAxisOrButton) {
    const InputManager manager;

    EXPECT_THROW((void)manager.getAxisRaw("Unknown"), std::out_of_range);
    EXPECT_THROW((void)manager.getButton("Unknown"), std::out_of_range);
    EXPECT_THROW((void)manager.getButtonDown("Unknown"), std::out_of_range);
    EXPECT_THROW((void)manager.getButtonUp("Unknown"), std::out_of_range);
}
