# Scene

The scene module builds the World from JSON files, like the scenes and prefabs
of Unity. A **configuration** file lists the scenes and gives the resources of
the whole game (the window, the input bindings), a **scene** file gives the
resources and entities of one level, and a **prefab** file describes one entity
(and its children) to reuse. The `SceneManager` reads them, adds the components
and resources to the World, and removes them when another scene is loaded.

It only creates data: the game still adds the systems that use it.

## Concepts

| Name              | Kind     | What it does                                                                                                                  |
| ----------------- | -------- | ----------------------------------------------------------------------------------------------------------------------------- |
| `SceneManager`    | class    | loads the configuration, the scenes and the prefabs, and knows how to read each component and resource                        |
| `Backends`        | resource | keeps alive the backends created from the files (`SfmlRenderer`, `SfmlAudioEngine`), added by the `SceneManager`              |
| `Serializers.hpp` | header   | the JSON conversions of the engine types (`Vec2f`, `Transform`, `Color`, `KeyCode`...)                                        |
| configuration     | file     | `configRoot`, `startScene`, the `scenes` by name, and the `resources` kept until the next configuration (see [Files](#files)) |
| scene             | file     | the `resources` and `entities` of one level, removed when another scene is loaded                                             |
| prefab            | file     | one entity: its `components` and `children` (see [Prefabs](#prefabs))                                                         |

Every file has `"version": 1` (`SceneManager::kVersion`): a file with another
version, or none, is refused.

The `SceneManager` only reads the types of the modules that are built: with
`AMPERSAND_MODULE_RENDER=OFF`, `Sprite` is an unknown key. Linking
`ampersand_scene` also links every module that is turned on.

## Example

`assets/config.json`:

```json
{
  "version": 1,
  "configRoot": "assets/",
  "startScene": "menu",
  "scenes": { "menu": "scenes/menu.json", "level1": "scenes/level1.json" },
  "resources": {
    "Renderer": {
      "backend": "sfml",
      "title": "R-Type",
      "size": { "x": 1280, "y": 720 },
      "vsync": true
    },
    "InputManager": { "buttons": { "Fire": ["Space", "Left"] } }
  }
}
```

`assets/scenes/level1.json`:

```json
{
  "version": 1,
  "name": "level1",
  "resources": {
    "ClearColor": { "value": { "r": 8, "g": 8, "b": 24, "a": 255 } },
    "Score": { "value": 0 }
  },
  "entities": [
    {
      "name": "Player",
      "prefab": "prefabs/ship.json",
      "components": { "Transform": { "position": { "x": 100, "y": 360 } } }
    }
  ]
}
```

`assets/prefabs/ship.json`:

```json
{
  "version": 1,
  "name": "Ship",
  "components": {
    "Transform": {
      "position": { "x": 0, "y": 0, "z": 0 },
      "scale": { "x": 1, "y": 1, "z": 1 },
      "rotationDegrees": { "x": 0, "y": 0, "z": 0 },
      "origin": { "x": 0, "y": 0, "z": 0 }
    },
    "Sprite": {
      "texture": "textures/ships.png",
      "source": { "x": 0, "y": 0, "width": 33, "height": 17 },
      "color": { "r": 255, "g": 255, "b": 255, "a": 255 },
      "layer": 0
    },
    "Velocity": { "value": { "x": 0, "y": 0 } }
  }
}
```

```cpp
namespace scene = ampersand::scene;

struct Score {  // a resource of the game
    int value{};
};
NLOHMANN_DEFINE_TYPE_NON_INTRUSIVE(Score, value)

ecs::World world;
scene::SceneManager scenes;               // the engine types are already registered
scenes.registerResource<Score>("Score");  // the game registers its own, before loading
scenes.loadConfig(world, "assets/config.json");  // opens the window, loads "menu"

auto& window = world.getResource<render::Renderer>();
while (window.isOpen) {
    scheduler.update(timer.tick());
    if (scenes.getCurrentScene() == "menu" &&
        world.getResource<input::InputManager>().getButtonDown("Fire")) {
        scenes.loadScene(world, "level1");  // the menu is removed, level1 is loaded
    }
}
```

The `Renderer` comes from the configuration, so the game never names the
backend: the `SceneManager` creates the `SfmlRenderer` and keeps it in the
`Backends` resource.

## Files

**Configuration**, opened with `loadConfig(world, path)`:

| Key          | Default | What it is                                                                     |
| ------------ | ------- | ------------------------------------------------------------------------------ |
| `version`    |         | version of the SceneManager to use                                             |
| `startScene` |         | name of the scene loaded at the end of `loadConfig`                            |
| `configRoot` | `""`    | folder every other path is relative to                                         |
| `scenes`     | `{}`    | scene name → scene file. `setScene(name, path)` adds or replaces one from code |
| `resources`  | `null`  | kept until the next `loadConfig`, see [Engine types](#engine-types)            |

**Scene**: `version`, then `resources` (default `null`) and `entities` (a list,
default `null`). Any other key (`name`) is ignored.

**Entity**, an item of `entities` or `children`:

| Key          | Default | What it is                                                                                              |
| ------------ | ------- | ------------------------------------------------------------------------------------------------------- |
| `prefab`     |         | prefab file the entity starts from (see [Prefabs](#prefabs))                                            |
| `components` | `{}`    | component name → its value                                                                              |
| `children`   | `[]`    | entities created as children of this one (see [Hierarchy](ecs.md#hierarchy))                            |
| `persistent` | `false` | kept across scenes (see [Persistent entities](#persistent-entities)), on the entries of `entities` only |
| `name`       |         | ignored: it only makes the file easier to read                                                          |

The path given to `loadConfig` is opened as it is. Every other path (scenes,
prefabs, textures, fonts, sounds) is relative to `configRoot`, which is itself
relative to the **working directory**, not to the configuration file.

Every field of a component is mandatory: there are no defaults, unless a prefab
gives them. A key of `components` or `resources` with no registered loader
throws (see [Types of the game](#types-of-the-game)).

!!! warning "Order of the keys"
The keys of a JSON object are read in **alphabetical order**, not in the
order of the file (`nlohmann::json` sorts them): the resources of a block
and the components of an entity. The `entities` and `children` lists keep
their order. So a loader never needs another key of its own block: the
`Renderer` a `Sprite` needs comes from the configuration, loaded before every
scene.

## Prefabs

A prefab file is an entity: `version`, `components` and `children`. An entry
with `"prefab"` starts from that file, then:

- its `components` are merged **field by field** over the ones of the prefab
  ([JSON Merge Patch](https://www.rfc-editor.org/rfc/rfc7386)): an object is
  merged into the object of the prefab, any other value (number, string,
  list) replaces it, and `null` removes the field or the whole component;
- its `children` are created after the children of the prefab.

```json
{
  "prefab": "prefabs/ship.json",
  "components": {
    "Transform": { "position": { "x": 100, "y": 360 } },
    "Velocity": null
  },
  "children": [{ "prefab": "prefabs/turret.json" }]
}
```

This ship keeps the `z` of its position, its `scale`, `rotationDegrees` and
`origin` from the prefab, has no `Velocity`, and gets a turret. A list (the
`frames` of an `Animation`) is always replaced whole.

A prefab can itself start from another prefab, which is applied first. A
prefab that uses itself, directly or not (as its base or in its children),
throws. The prefab files are read from the disk each time they are used.

`instantiate` creates a prefab from code, children included, and returns its
root:

```cpp
const ecs::Entity bullet = scenes.instantiate(world, "prefabs/bullet.json");
world.get<core::Transform>(bullet).position = muzzle;
```

The entity belongs to the current scene: the next `loadScene` destroys it,
unless it is [persistent](#persistent-entities).

## Loading

`loadScene(world, name)`:

1. finds the scene and reads its file. An unknown scene, a missing file, bad
   JSON or a bad version throw here, and the current scene is left as it was;
2. destroys the entities of the current scene (the ones it created and the
   instantiated ones, not the persistent ones), then removes its resources,
   the last loaded first;
3. loads the resources of the new scene, then its entities.

`loadConfig(world, path)` reads the file and checks its `startScene` (a string,
listed in `scenes`), removes everything the previous configuration loaded (its
scenes, its persistent entities and its resources), loads the `resources` of
the configuration, then its `startScene`.

| What                                       | Created by                                | Removed by                                 |
| ------------------------------------------ | ----------------------------------------- | ------------------------------------------ |
| resources of the configuration             | `loadConfig`                              | the next `loadConfig`                      |
| resources of the scene                     | `loadScene`                               | the next `loadScene`                       |
| entities of the scene                      | `loadScene`, `instantiate`                | the next `loadScene`                       |
| persistent entities                        | `"persistent": true`, `dontDestroyOnLoad` | the next `loadConfig`                      |
| asset ids (textures, fonts, sounds, music) | their first use (see [Assets](#assets))   | removing the `Renderer` or `AudioListener` |

The `SceneManager` only removes what it created: the entities created with
`world.create()` and the resources added by the game stay, except an entity the
game marked with `dontDestroyOnLoad`, which the next `loadConfig` destroys like
the other persistent entities. An entity destroyed
by the game is skipped. A resource key belongs to the configuration **or** to
the scenes: if both give it, the scene fails, since `addResource` throws when
the resource is already there.

`loadScene` destroys entities: call it outside the loops over `owners<T>()` or
`children(e)` (between two `scheduler.update`, as in the example).

## Persistent entities

Like `DontDestroyOnLoad` in Unity, a persistent entity survives `loadScene`. It
is destroyed by the next `loadConfig`, or by the game.

- `"persistent": true` on an entry of `entities` creates it the first time its
  scene is loaded. Loading that scene again does not create a second one while
  the first is alive (it is created again if it was destroyed). The entry is
  known by its scene and its **index** in `entities`: moving the entries in the
  file changes which one it is. On a child, or in a prefab file, the key is
  ignored.
- `dontDestroyOnLoad(world, entity)` does the same from code. The entity must
  be a root (`std::invalid_argument` for a child), and its children go with it.

An entity attached under a persistent one with `setParent` follows it too, even
one created by the scene: `loadScene` only destroys the entities whose root is
not persistent.

```cpp
const ecs::Entity player = scenes.instantiate(world, "prefabs/player.json");
scenes.dontDestroyOnLoad(world, player);  // follows the player from level to level
```

## Engine types

The engine types are registered when the `SceneManager` is created, for the
modules that are built. The values use these formats, every field mandatory:

| Type            | JSON                                                                                                             |
| --------------- | ---------------------------------------------------------------------------------------------------------------- |
| `Vec2`, `Vec3`  | `{ "x", "y" }`, `{ "x", "y", "z" }`                                                                              |
| `Rect`          | `{ "x", "y", "width", "height" }`                                                                                |
| `Color`         | `{ "r", "g", "b", "a" }`, from 0 to 255                                                                          |
| input `Binding` | a name of [Input](input.md#limits): `"Space"`, `"Left"`, `"Button0"`, or `null` for none. An unknown name throws |

**Resources:**

| Key             | Module  | Fields                                                                                      |
| --------------- | ------- | ------------------------------------------------------------------------------------------- |
| `Renderer`      | render  | `"backend": "sfml"`, `size`, `title`, `vsync`: opens the window (see [Backends](#backends)) |
| `Camera`        | render  | `center`, `size`                                                                            |
| `ClearColor`    | render  | `value`                                                                                     |
| `GravityForce`  | physics | `value`                                                                                     |
| `InputManager`  | input   | `axes` (name → `Axis`) and `buttons` (name → list of bindings), both optional               |
| `AudioListener` | audio   | `"backend": "sfml"`, `volume` (optional, 100)                                               |
| `SceneMusic`    | audio   | `music` (a path), `volume`, `loop`, `playing` (optional: 100, `true`, `false`)              |

An `Axis` has `positive`, `negative` (bindings), `joystick` (a `JoystickAxis`
name such as `"X"`, or `null`; an unknown name throws), `dead` and
`invertJoystick`.

**Components:**

| Key                     | Module  | Fields                                                                             |
| ----------------------- | ------- | ---------------------------------------------------------------------------------- |
| `Transform`             | core    | `position`, `scale`, `rotationDegrees`, `origin` (each a `Vec3`)                   |
| `Sprite`                | render  | `texture` (a path), `source`, `color`, `layer`                                     |
| `Text`                  | render  | `font` (a path), `value`, `size`, `color`, `layer`                                 |
| `Animation`             | render  | `frames` (list of `Rect`), `frameDuration`, `loop`                                 |
| `AnimationStateMachine` | render  | `initial`, `states`, `params`, `transitions` (see [State machine](#state-machine)) |
| `Velocity`              | physics | `value`                                                                            |
| `Gravity`               | physics | `scale`                                                                            |
| `BoxCollider`           | physics | `bounds`                                                                           |
| `AudioSource`           | audio   | `sound` **or** `music` (a path, exactly one), `volume`, `loop`, `playing`          |

A `Sprite` or `Text` needs the `Renderer`, and an `AudioSource` or `SceneMusic`
needs the `AudioListener`.

`Renderer`, `InputManager` and `AudioListener` are only allowed in the
`resources` of the **configuration** (`ResourceScope::Config`): the
`RenderSystem`, `InputSystem` and `AudioSystem` need them in every scene. A scene giving one fails before anything is unloaded:

```text
Failed to load scene 'level1': Resource 'Renderer' is only allowed in the configuration
```

### Backends

`Renderer` and `AudioListener` create the backend named by `"backend"` (only
`"sfml"` for now) and keep it in the `Backends` resource, under their own key.
Removing them removes their backend too: the window closes, and their assets
are forgotten. Since they come from the configuration, the window stays open and the textures stay loaded from one scene to the next.

A new `AudioListener` (from the next `loadConfig`) has a new engine, whose ids start again at 1, while the `AudioSystem` keeps the ids of the old one: the new `SceneMusic` can be taken for the track already playing and never start. Create a new `AudioSystem` after a `loadConfig` that loads an `AudioListener` again.

### Assets

The textures, fonts, sounds and music are given by path, and each path is
loaded once: two `Sprite`s with the same `texture` share one `TextureId`. A file
that cannot be loaded throws. The ids are kept until their backend is removed.

### State machine

The `AnimationStateMachine` of [Render](render.md#state-machine) is written
with conditions instead of functions:

```json
"AnimationStateMachine": {
    "initial": "idle",
    "states": { "idle": { ... }, "jump": { ... } },
    "params": { "floats": { "speedY": 0 }, "bools": { "grounded": true } },
    "transitions": [
        { "from": "*", "to": "jump", "when": [{ "param": "speedY", "op": "<", "value": 0 }] },
        { "from": "jump", "to": "idle", "when": [{ "param": "grounded", "value": true }] }
    ]
}
```

- each state is an `Animation`;
- a transition is taken when **every** condition of `when` holds (an empty
  list always holds);
- a float condition compares the param with `op`: `<`, `<=`, `>`, `>=`, `==`
  or `!=`. A param missing from `floats` reads 0;
- a bool condition has no `op`: it holds when the param equals `value`. A param
  missing from `bools` never holds;
- an unknown `op`, or an `op` on a bool condition, throws when the component
  is loaded.

## Types of the game

The game registers its own types, **before** `loadConfig`, so the files can use
them. Registering a key again replaces its loader, the built-in ones included.

| Call                                   | Builds                                                                                         |
| -------------------------------------- | ---------------------------------------------------------------------------------------------- |
| `registerComponent<T>(name)`           | `world.add(entity, data.get<T>())`                                                             |
| `registerComponent(name, load)`        | calls `load(world, entity, data)`                                                              |
| `registerResource<T>(name)`            | `world.addResource<T>(data.get<T>())`                                                          |
| `registerResource<T>(name, load)`      | calls `load(world, data)`                                                                      |
| `registerResource<T, Data>(name, dep)` | `world.addResource<T>(dep, data.get<Data>())`, for a resource built from an object of the game |
| `registerResource(name, load, remove)` | calls `load(world, data)`, and `remove(world)` when it is unloaded                             |

Each `registerResource` takes a last, optional `ResourceScope`:
`ResourceScope::Any` (default) lets the configuration and the scenes give the resource, `ResourceScope::Config` only the configuration, like the `Renderer`.

```cpp
scenes.registerResource<Settings>("Settings", scene::ResourceScope::Config);
```

The resources registered with a type `T` are removed with
`world.removeResource<T>()`. A `dep` is kept by reference: it must outlive the
`SceneManager` and the World.

`data.get<T>()` needs a JSON conversion of `T`: `NLOHMANN_DEFINE_TYPE_NON_INTRUSIVE`
(every field mandatory) or a `from_json` of its own. The conversions of the
engine types are in `Serializers.hpp`, included by `SceneManager.hpp`, so a
field of type `Vec2f` or `Color` works as it is.

```cpp
struct Health {
    int value{};
    int max{};
};
NLOHMANN_DEFINE_TYPE_NON_INTRUSIVE(Health, value, max)

scenes.registerComponent<Health>("Health");  // { "value": 3, "max": 3 }

// or a loader of its own: only "max", the health starts full
scenes.registerComponent("Health", [](ecs::World& world, ecs::Entity entity,
                                      const scene::json& data) {
    const auto max = data.at("max").get<int>();
    world.add(entity, Health{.value = max, .max = max});
});
```

A loader that needs a file uses `asset(kind, path, load)`: `load` receives the
full path and returns the id (0 on failure), and is only called the first time
a path is used. With the kind `"texture"`, the cache is shared with the
`Sprite`s and forgotten with the `Renderer`. A kind of its own is forgotten with
`forgetAssets(kind)`.

```cpp
scenes.registerComponent("Parallax", [&scenes](ecs::World& world, ecs::Entity entity,
                                               const scene::json& data) {
    auto& renderer = world.getResource<render::Renderer>().renderer;
    const auto texture = scenes.asset(
        "texture", data.at("texture").get<std::string>(),
        [&renderer](const std::string& path) { return renderer.loadTexture(path); });
    world.add(entity, Parallax{.texture = texture, .speed = data.at("speed").get<float>()});
});
```

## Errors

Every error is a `std::runtime_error` (`std::invalid_argument` for
`dontDestroyOnLoad`), and its message gives the path to the faulty value:

```text
Failed to load scene 'level1': Failed to load component 'Sprite': [json.exception.out_of_range.403] key 'layer' not found
```

An unknown scene, a missing file, bad JSON, a bad version, a missing or unknown `startScene`, or a scene giving a resource only allowed in theconfiguration are found before anything is unloaded: the previous scene (or configuration) stays as it was. Any other error (a missing field, a key with no loader, an asset that cannot be loaded, an unknown backend, input name or operator) is found once the previous one is unloaded, so the load is undone:

- `loadScene` destroys every entity the new scene created (its persistent ones
  too) and removes its resources. The configuration stays, and
  `getCurrentScene()` is empty;
- `loadConfig` also removes everything its configuration loaded, so the World
  holds nothing from any configuration (only what the game added itself).

## Limits

| Rule       | Value                                                                                                              |
| ---------- | ------------------------------------------------------------------------------------------------------------------ |
| Version    | only `1`, for every file                                                                                           |
| Backends   | only `"sfml"`, for the `Renderer` and the `AudioListener`                                                          |
| Order      | the keys of an object are read in alphabetical order                                                               |
| Fields     | every field of a component is mandatory (only `AudioListener`, `SceneMusic` and `InputManager` have optional ones) |
| Types      | only the ones of the tables above: the others (`Clock`...) are added by code                                       |
| Prefabs    | read from the disk on each use, no cache                                                                           |
| Persistent | an entry is known by its index in `entities`                                                                       |
| Saving     | the files are only read: the World cannot be written back to JSON                                                  |
| Paths      | relative to `configRoot`, itself relative to the working directory                                                 |
