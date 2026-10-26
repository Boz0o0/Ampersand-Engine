# Scripting

The scripting module gives entities a behaviour written in Lua (run through
[sol2](https://github.com/ThePhD/sol2)), without recompiling the game: the
`Scripts` component lists the `.lua` files of an entity, and the
`ScriptingSystem` runs them every frame.

## Concepts

| Name                      | Kind      | What it does                                                             |
| ------------------------- | --------- | ------------------------------------------------------------------------ |
| `Scripts`                 | component | the paths of the `.lua` files of the entity (`scripts`)                  |
| `ScriptingSystem`         | system    | loads the files, creates one instance per script and calls its functions |
| `ScriptError`             | event     | an error of a script: `path` of the file and `message`                   |
| `EventQueue<ScriptError>` | resource  | where the errors are sent, optional                                      |
| `CommandQueue`            | resource  | where `destroy` records the entities (added by the `Scheduler`)          |

The `ScriptingSystem` reads `dt` from the `Clock` resource, and throws
`ResourceNotFound` without it.

## Example

A script returns a table, given to its functions as `self`:

```lua
-- scripts/ship.lua
local Ship = {}

function Ship:start(dt)
    self.speed = 300
end

function Ship:update(dt)
    local transform = getTransform(self.entity)
    transform.position.x = transform.position.x
        + input.getAxis("Horizontal") * self.speed * dt
    if transform.position.x > 1920 then
        destroy(self.entity)
    end
end

function Ship:onDestroy(dt)
    print("ship gone")
end

return Ship
```

```cpp
namespace scripting = ampersand::scripting;

scheduler.addEventQueue<scripting::ScriptError>();  // optional, to read errors
scheduler.add<scripting::ScriptingSystem>();
world.add(ship, scripting::Scripts{{"scripts/ship.lua", "scripts/hum.lua"}});
```

With the [scene module](scene.md), scenes and prefabs give them in their
`components`, with paths relative to `gameRoot`. A path given from C++ is used
as it is (relative to the folder the game is run from).

```json
"Scripts": { "scripts": ["scripts/ship.lua", "scripts/hum.lua"] }
```

An entity gets one **instance** of each of its scripts: the returned table, with
`self.entity` set. What a script stores in `self` belongs to that instance, so
50 enemies running `enemy.lua` have 50 different `self`. A file is compiled once,
the first time it is used.

## Engine API

### Types

| Lua name      | C++ type                  | Fields                                                    | Created from Lua                             |
| ------------- | ------------------------- | --------------------------------------------------------- | -------------------------------------------- |
| `Entity`      | `core::ecs::Entity`       | `index` (read only)                                       | no                                           |
| `Vec2`        | `core::math::Vec2f`       | `x`, `y`                                                  | `Vec2.new()`, `Vec2.new(x, y)`               |
| `Vec3`        | `core::math::Vec3f`       | `x`, `y`, `z`                                             | `Vec3.new()`, `Vec3.new(x, y, z)`            |
| `Rect`        | `core::math::Rect<float>` | `x`, `y`, `width`, `height`, `intersects(rect)`           | `Rect.new()`, `Rect.new(x, y, width, height)` |
| `Transform`   | `core::Transform`         | `position`, `scale`, `rotationDegrees`, `origin` (`Vec3`) | no                                           |
| `BoxCollider` | `physics::BoxCollider`    | `bounds` (`Rect`, local to the entity)                    | no                                           |
| `Velocity`    | `physics::Velocity`       | `value` (`Vec2`, per second)                              | no                                           |
| `Text`        | `render::Text`            | `value`, `size`, `color`, `layer`                         | no                                           |
| `Color`       | `render::Color`           | `r`, `g`, `b`, `a` (0 to 255)                             | no                                           |
| `AudioSource` | `audio::AudioSource`      | `volume`, `loop`, `playing`                               | no                                           |

### Functions

| Function                                  | Module  | Returns              | What it does                                                         |
| ----------------------------------------- | ------- | -------------------- | -------------------------------------------------------------------- |
| `getTransform(entity)`                    | core    | `Transform`          | the component itself: changing it changes the entity                 |
| `hasTransform(entity)`                    | core    | boolean              | whether the entity has a `Transform`                                 |
| `destroy(entity)`                         | core    |                      | destroys the entity at the end of the frame                          |
| `getParent(entity)`                       | core    | `Entity`             | the parent, `nil` for a root                                         |
| `getChildren(entity)`                     | core    | list of `Entity`     | the children, in the order they were attached                        |
| `input.getAxis(name)`                     | input   | number               | value of the axis, from -1 to 1                                      |
| `input.getButton(name)`                   | input   | boolean              | whether the virtual button is held                                   |
| `input.getButtonDown(name)`               | input   | boolean              | whether it was pressed during this frame                             |
| `input.getButtonUp(name)`                 | input   | boolean              | whether it was released during this frame                            |
| `getBoxCollider(entity)`                  | physics | `BoxCollider`        | the `BoxCollider` of the entity                                      |
| `hasBoxCollider(entity)`                  | physics | boolean              | whether the entity has a `BoxCollider`                               |
| `getBounds(entity)`                       | physics | `Rect`               | the box in the world, from its `BoxCollider` and `Transform`         |
| `getColliders()`                          | physics | list of `Entity`     | the entities with a `BoxCollider` and a `Transform`                  |
| `getVelocity(entity)`                     | physics | `Velocity`           | the `Velocity` of the entity, applied by the `MovementSystem`        |
| `hasVelocity(entity)`                     | physics | boolean              | whether the entity has a `Velocity`                                  |
| `getText(entity)`                         | render  | `Text`               | the `Text` of the entity                                             |
| `hasText(entity)`                         | render  | boolean              | whether the entity has a `Text`                                      |
| `animation.setBool(entity, name, value)`  | render  |                      | sets a boolean parameter of the `AnimationStateMachine`              |
| `animation.setFloat(entity, name, value)` | render  |                      | sets a number parameter                                              |
| `animation.playOnce(entity, action)`      | render  |                      | plays the state `action` once, then goes back                        |
| `animation.isPlayingAction(entity)`       | render  | boolean              | whether an action of `playOnce` still plays                          |
| `animation.getState(entity)`              | render  | string               | the current state (the base state while an action plays)             |
| `getAudioSource(entity)`                  | audio   | `AudioSource`        | the `AudioSource` of the entity                                      |
| `hasAudioSource(entity)`                  | audio   | boolean              | whether the entity has an `AudioSource`                              |

The functions and types of a module (`Velocity`, `Text`, `AudioSource`...)
only exist when it is turned on: `input` is `nil` without `AMPERSAND_MODULE_INPUT`. Calling one on
an entity without the component (or `input` without an `InputManager`, an
unknown axis, button or state) raises an [error](#errors).

!!! warning "Do not keep a component"
    `getTransform` and the other getters give the component itself, which can
    move in memory when components are added or removed. It is only valid
    during the current call: never keep it (or one of its `Vec3`) in `self`,
    call the getter again.

- **`destroy`** keeps the entity alive until the end of the frame (it goes
  through the `CommandQueue`), and its `onDestroy` is called on the next update.
- **Input**: the axes and buttons are the ones declared with `setAxis` and
  `setButton`, see [Input](input.md).
- **Collisions**: a script checks them itself. Unlike the `CollisionSystem`
  ([Physics](physics.md)), `intersects` does not compare the `z`.

    ```lua
    function Missile:update(dt)
        local bounds = getBounds(self.entity)
        for _, other in ipairs(getColliders()) do
            if other.index ~= self.entity.index and bounds:intersects(getBounds(other)) then
                destroy(other)
                destroy(self.entity)
            end
        end
    end
    ```

- **Animation** drives the `AnimationStateMachine`, see [Render](render.md).
- **Sound**: the sound comes from the `AudioSource` (given in JSON, see
  [Audio](audio.md)), played by the `AudioSystem` when `playing` is set.
  `playing` goes back to `false` when a sound that does not loop ends. An entity
  has one `AudioSource`: give each other sound to a child (the `children` of a
  prefab), and each shot of a rapid fire its own entity.

    ```lua
    function Ship:update(dt)
        local children = getChildren(self.entity)  -- gun, then engine
        if input.getButtonDown("Fire") then
            getAudioSource(children[1]).playing = true
        end
        getAudioSource(children[2]).playing = input.getAxis("Horizontal") ~= 0
    end
    ```

### Bindings of the game

The game adds its own types and functions with the sol2 API on `lua()`, before
the scripts using them run. Do not reuse the names of the engine
(`getTransform`, `input`, `Vec3`...): they would replace them.

A binding changing the World too much to run during the scripts (creating many
entities, adding or removing `Scripts`) gives its work to `defer`: it runs at the
end of the update, after every script, and its error is a `ScriptError` of the
script that called the binding.

```cpp
auto& scripts = scheduler.add<scripting::ScriptingSystem>();
scripts.lua().new_usertype<Health>("Health", sol::no_constructor,
                                   "value", &Health::value);
scripts.lua().set_function("getHealth", [&world](ecs::Entity entity) -> Health& {
    return world.get<Health>(entity);
});
scripts.lua().set_function("spawnWave", [&scripts] {
    scripts.defer([](ecs::World& world) { /* create the enemies */ });
});
```

## Functions called

Every function is optional, and receives `self` and `dt` (seconds). On each
update, the `ScriptingSystem`:

1. calls `onDestroy` on the scripts of the entities destroyed or without
   `Scripts`, then forgets them. The entity is already gone: `self.entity` only
   identifies it. It is called even if `start` never ran;
2. for each entity with `Scripts`, in the order of its list: `start` the first
   time, then `update`;
3. applies the actions given to `defer`.

When the list of an entity changes (a path added, removed or moved), all its
instances are rebuilt: `onDestroy` on the old ones, `start` on the new ones,
which lose what they stored in `self`.

## Globals

Each instance has its own globals: `count = 1` is not seen by other scripts,
nor by other instances of the same file. The libraries `base` (`print`,
`tostring`...), `math`, `string` and `table` are shared and readable, but only
assignments are kept apart: `math.foo = 1` changes it for every script.

## Errors

A script never stops the game. It is turned off (its functions are no longer
called) and a `ScriptError` is sent when its file is missing, has a syntax
error, does not return a table or raises an error when it runs, or when one of
its functions (or a binding it calls) raises an error. The other scripts keep
running, and a script turned off stays off until the list of its entity
changes.

The errors go to the `EventQueue<ScriptError>` if the game added it, and are
dropped otherwise. Read them in a system run after the `ScriptingSystem`: the
`Scheduler` empties the queue at the end of the frame.

```cpp
class ShowScriptErrors final : public ampersand::core::ecs::ISystem {
public:
    void update(ampersand::core::ecs::World& world) override {
        auto& errors = world.getResource<
            ampersand::core::ecs::EventQueue<scripting::ScriptError>>();
        for (const auto& error : errors.read()) {
            std::cerr << error.message << std::endl;
        }
    }
};
```

## Limits

| Rule       | Value                                                                                |
| ---------- | ------------------------------------------------------------------------------------ |
| Engine     | only the types above are bound: the game binds the rest with `lua()`                 |
| Paths      | relative to `gameRoot` in JSON, to the folder the game is run from in C++            |
| List       | changing the list of an entity resets all its scripts                                |
| Globals    | kept apart per instance, but shared tables (`math`...) are not                       |
| Components | a component given by a getter is only valid during the current call                  |
| Collisions | `intersects` does not compare the `z`                                                |
