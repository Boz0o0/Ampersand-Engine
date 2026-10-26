# Scripting

The scripting module gives entities a behaviour written in Lua, without
recompiling the game. The game lists the `.lua` files of an entity in its
`Scripts` component, and the `ScriptingSystem` runs them every frame. Lua is
run through [sol2](https://github.com/ThePhD/sol2).

## Concepts

| Name                       | Kind      | What it does                                                            |
| -------------------------- | --------- | ----------------------------------------------------------------------- |
| `Scripts`                  | component | the paths of the `.lua` files of the entity (`scripts`)                 |
| `ScriptingSystem`          | system    | loads the files, creates one instance per script and calls its functions |
| `ScriptError`              | event     | an error raised by a script: `path` of the file and `message`           |
| `EventQueue<ScriptError>`  | resource  | where the errors are sent, optional                                     |

The `ScriptingSystem` reads `dt` from the `Clock` resource and throws
`ResourceNotFound` without it.

## Example

A script returns a table. Its functions are called with that table as `self`:

```lua
-- scripts/blink.lua
local Blink = {}

function Blink:start(dt)
    self.time = 0
end

function Blink:update(dt)
    self.time = self.time + dt
end

function Blink:onDestroy(dt)
    print("entity gone after", self.time, "seconds")
end

return Blink
```

```cpp
namespace scripting = ampersand::scripting;

scheduler.addEventQueue<scripting::ScriptError>();  // optional, to read errors
scheduler.add<scripting::ScriptingSystem>();

const auto lamp = world.create();
world.add(lamp, scripting::Scripts{{"scripts/blink.lua", "scripts/hum.lua"}});
```

Paths are relative to the folder the game is run from.

## Script instances

An entity gets one **instance** of each of its scripts: the table returned by
the file, with `self.entity` set to the entity. Everything a script stores in
`self` belongs to that instance, so 50 enemies running `enemy.lua` have 50
different `self`.

A file is compiled once, the first time it is used, then every instance is
created from that compiled code.

## Functions called

Every function is optional, and receives `self` and `dt` (the time of the frame,
in seconds). On each update, the `ScriptingSystem`:

1. calls `onDestroy` on the scripts of the entities that were destroyed or lost
   their `Scripts`, then forgets them;
2. for each entity with `Scripts`, in the order of its list:
    - calls `start` the first time the script runs;
    - calls `update`.

When the list of an entity changes (a path added, removed or moved), all its
instances are rebuilt: `onDestroy` is called on the old ones and `start` on the
new ones, so they lose what they stored in `self`.

!!! note "onDestroy"
    When an entity is destroyed, `onDestroy` is called on the next update: the
    entity is already gone, `self.entity` is only kept to identify it.
    `onDestroy` is also called on a script whose `start` never ran.

## Globals

Each instance has its own globals: a script that sets `count = 1` only sets it
for itself, other scripts (and other instances of the same file) do not see it.
The Lua libraries are shared and can still be read: `base` (`print`,
`tostring`...), `math`, `string` and `table`.

Only assignments are kept apart: changing a shared table (`math.foo = 1`)
changes it for every script.

## Errors

A script never stops the game. When one of these happens, the script is turned
off (its functions are no longer called) and a `ScriptError` is sent:

- the file is missing or has a syntax error;
- the file does not return a table, or raises an error when it runs;
- `start`, `update` or `onDestroy` raises an error.

The other scripts, of the same entity or not, keep running. The errors are sent
to the `EventQueue<ScriptError>` resource if the game added it, and dropped
otherwise. Read them in a system that runs after the `ScriptingSystem`, since
the `Scheduler` empties the queue at the end of the frame:

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

A script turned off stays off, until the list of its entity changes.

## Limits

| Rule     | Value                                                                  |
| -------- | ---------------------------------------------------------------------- |
| Engine   | scripts only know `self.entity` and `dt`: no access to the components |
| Paths    | relative to the folder the game is run from                             |
| List     | changing the list of an entity resets all its scripts                  |
| Globals  | kept apart per instance, but shared tables (`math`...) are not          |
