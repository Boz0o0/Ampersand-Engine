# Input

The input module gives the keyboard, mouse and controllers of the frame in
one resource: what is held, what was pressed and released, and where the mouse
is. It does not know the window, so it runs with any source of events (the window of the
render module, a replay, a test).

## Concepts

| Name             | Kind     | What it does                                                                                                                                                                                                            |
| ---------------- | -------- | ----------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------- |
| `InputManager`   | resource | the state of the frame, read with the `get` functions (or `keysDown()`, `keysPressed()`, `mouse()`...), the axes and buttons of the game (`setAxis`, `setButton`), and `beginFrame` / `apply` used by the `InputSystem` |
| `Event`          | type     | what the poll function gives the `InputSystem`: one of `KeyPressed`, `KeyReleased`, `MousePressed`, `MouseReleased`, `MouseMoved`, `JoystickButtonPressed`, `JoystickButtonReleased`, `JoystickMoved`, `JoystickDisconnected`, `FocusLost` |
| `KeyCode`        | enum     | a key, by its **physical position** on the keyboard (see [Limits](#limits))                                                                                                                                             |
| `MouseButton`    | enum     | a mouse button: `Left`, `Right`, `Middle`                                                                                                                                                                               |
| `JoystickButton` | enum     | a controller button, by index: `Button0` to `Button31`                                                                                                                                                                  |
| `JoystickAxis`   | enum     | a controller axis: `X`, `Y`, `Z`, `R`, `U`, `V`, `PovX`, `PovY`                                                                                                                                                         |
| `Binding`        | type     | a `KeyCode`, a `MouseButton`, a `JoystickButton`, or nothing (`std::monostate`), used by the axes and virtual buttons                                                                                                   |
| `Axis`           | struct   | a `positive` and a `negative` binding, and an optional controller axis, read by name with `getAxisRaw`                                                                                                                  |
| `InputSystem`    | system   | fills the `InputManager` with the events given by a poll function                                                                                                                                                       |

The `InputSystem` throws `ResourceNotFound` if the World has no `InputManager`.

## Example

```cpp
namespace input = ampersand::input;

world.addResource<input::InputManager>();
input::InputSystem inputSystem(pollWindow);
PlayerSystem player;                         // a system of the game
render::RenderSystem draw;

while (world.getResource<render::Renderer>().isOpen) {
    inputSystem.update(world);  // first, so the other systems see this frame
    player.update(world);
    draw.update(world);         // last: it displays the frame
}
```

## Reading the input

The names follow the [Input Manager of Unity](https://docs.unity3d.com/Manual/class-InputManager.html).
Like in Unity, the game never reads events: it reads a state. There are three
layers, each built from the one before:

1. **events**: what happened during the frame, given by the window to the
   `InputSystem` only;
2. **state**: what is held (`keysDown()`, `buttonsDown()`,
   `joystickButtonsDown()`), what was pressed and released during the frame
   (`keysPressed()`, `keysReleased()`, `buttonsPressed()`...), `mouse()` and
   `joystickAxes()`, kept up to date by the `InputSystem` from the events and
   read only for the game;
3. **get functions**: what the game reads, computed from the state.

A key pressed and released during the same frame is in `keysPressed` and in
`keysReleased`: `getKeyDown` and `getKeyUp` are both true, `getKey` is false.

```cpp
const auto& manager = world.getResource<input::InputManager>();
```

**Keys, mouse buttons and controller buttons:**

| Function                                         | True when                         |
| ------------------------------------------------ | --------------------------------- |
| `getKey(key)` / `getMouseButton(button)`         | it is held                        |
| `getKeyDown(key)` / `getMouseButtonDown(button)` | it was pressed during this frame  |
| `getKeyUp(key)` / `getMouseButtonUp(button)`     | it was released during this frame |

The controller buttons have the same three functions: `getJoystickButton`,
`getJoystickButtonDown` and `getJoystickButtonUp`. `getJoystickAxis(axis)`
gives the position of a controller axis, from `-1` to `1` (`0` before it
moves).

```cpp
if (manager.getKey(input::KeyCode::LeftArrow)) {
    position.x -= speed * dt;
}
if (manager.getMouseButtonDown(input::MouseButton::Left)) {
    aimAt(manager.mouse());
}
```

**Axes and virtual buttons:** the game names them once, then reads them by
name, so the keys can change without touching the systems that read them.

```cpp
auto& manager = world.getResource<input::InputManager>();
manager.setAxis("Horizontal", {.positive = input::KeyCode::D,
                               .negative = input::KeyCode::A});
manager.setAxis("Vertical", {.positive = input::KeyCode::W,
                             .negative = input::KeyCode::S,
                             .joystick = input::JoystickAxis::Y,
                             .invertJoystick = true});  // the stick is down > 0
manager.setButton("Fire", {input::KeyCode::Space, input::MouseButton::Left,
                           input::JoystickButton::Button0});

// in a system
position.x += manager.getAxisRaw("Horizontal") * speed * dt;  // -1, 0 or 1
position.y -= manager.getAxisRaw("Vertical") * speed * dt;    // -1 to 1
if (manager.getButtonDown("Fire")) {
    shoot();
}
```

- a binding is a key, a mouse button or a controller button, read with
  `getBinding`, `getBindingDown` and `getBindingUp`; a binding left empty is
  never held;
- `getAxisRaw` gives `1` while the positive binding is held, `-1` while the
  negative binding is held, `0` when both or none are. With a `joystick` axis,
  it also reads its position (`0` closer to `0` than `dead`, `0.2` by default,
  negated with `invertJoystick`), and keeps the value of the two that is the
  farthest from `0`, like Unity;
- `getButton`, `getButtonDown` and `getButtonUp` are true when one of the
  bindings of the button is;
- `setAxis` and `setButton` replace the axis or button of the same name;
- an axis or button name that was not set throws `std::out_of_range`.

## Filling the input

The `InputSystem` is given a **poll function** when it is created: it receives
the World, and each call returns the next input event, or `std::nullopt` when
there is none left.

Each frame, `update` calls `beginFrame` on the `InputManager` (it empties the
`Pressed` and `Released` sets), then calls the poll function until it returns
`std::nullopt` and gives each event to `apply`:

- a press of a key or a button that is already held is ignored (the keyboard
  repeats a held key);
- the keys and buttons that are `None` are ignored: they are never held;
- `FocusLost` releases every held key and mouse button (they are in the
  `Released` sets of the frame). The controllers are not touched;
- `JoystickDisconnected` forgets the held buttons and the axes of the
  controller: its axes are 0 again, and its buttons are no longer held (they
  are not in the `Released` sets).

With the render module, the poll function reads the window of the `Renderer`
resource (see [Render](render.md#window)). It also receives the other events of
the window, so it handles them there: `Closed` sets `isOpen` to `false`, and the
`RenderSystem` closes the window.

```cpp
std::optional<input::Event> pollWindow(ecs::World& world) {
    auto& window = world.getResource<render::Renderer>();
    while (const auto event = window.renderer.pollEvent()) {
        const auto forwarded = std::visit(
            [&window](const auto& value) -> std::optional<input::Event> {
                using Type = std::decay_t<decltype(value)>;
                if constexpr (std::is_same_v<Type, render::Closed>) {
                    window.isOpen = false;
                } else if constexpr (std::is_constructible_v<input::Event,
                                                             Type>) {
                    return value;
                }
                return std::nullopt;
            },
            *event);
        if (forwarded) {
            return forwarded;
        }
    }
    return std::nullopt;
}
```

This function is the only one that polls the window: it replaces the
`pollEvent()` loop of the game. An event polled anywhere else is lost for the
others.

## Events of the other modules

The engine has no event bus. An event is data of the frame in a resource of the
World: the module that produces it fills the resource, and the systems that run
after it read it. The `Pressed` and `Released` sets of the `InputManager`
follow this rule, and so does the `Collisions` resource of
[Physics](physics.md):

```cpp
void PlayerSystem::update(ecs::World& world) {
    for (const auto& [first, second] :
         world.getResource<physics::Collisions>().pairs) { ... }
}
```

The game adds its own events the same way, with a resource of its own
(`struct EnemyDeaths { std::vector<ecs::Entity> entities; };`) that it empties
at the start of each frame.

## Limits

| Rule       | Value                                                                                                                                                                                                                                                                                  |
| ---------- | -------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------- |
| Keys       | `A`-`Z`, `Alpha0`-`Alpha9`, `UpArrow`, `DownArrow`, `LeftArrow`, `RightArrow`, `Space`, `Return`, `Escape`, `Tab`, `Backspace`, `LeftShift`, `RightShift`, `LeftControl`, `RightControl`, `LeftAlt`, `RightAlt`. Any other key is `None`                                               |
| Letters    | a key is a physical position, named after QWERTY: on AZERTY, the key labelled `Z` is `KeyCode::W`, so `WASD` is the same place on every keyboard                                                                                                                                       |
| Mouse      | `Left`, `Right` and `Middle` buttons (others are `None`), position in window pixels: without a `Camera`, the same as world units                                                                                                                                                       |
| Wheel      | not reported                                                                                                                                                                                                                                                                           |
| Order      | the order of the inputs inside a frame is not kept: only what is held, pressed and released                                                                                                                                                                                            |
| Axes       | raw only (`getAxisRaw`): no smoothing or sensitivity, a dead zone for the controller axis only                                                                                                                                                                                         |
| Controller | buttons by index (`Button0`-`Button31`, others are `None`): which button is behind an index depends on the controller. The state is kept per controller, but read as one: a button is held if any controller holds it, an axis is the position farthest from `0` among the controllers |
| Focus      | losing the focus releases every held key and mouse button, so a key still held when the focus comes back must be pressed again. The controllers are not touched: SFML still reports them without the focus                                                                             |
