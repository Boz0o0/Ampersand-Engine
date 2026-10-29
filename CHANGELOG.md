# Changelog

All notable changes to the engine are listed here. The format follows
[Keep a Changelog](https://keepachangelog.com/en/1.1.0/), and versions follow
[Semantic Versioning](https://semver.org/).

Everything here is written by hand.

**When making a pull request**, please add a short description of your changes
under `[Unreleased]`, in the right subsection (`Added`, `Changed`, `Fixed` or
`Removed`, create it if it is missing).

**When releasing a new version**:

1. In a pull request, rename `## [Unreleased]` to the new version and today's
   date (eg: `## [0.2.0] - 2026-10-16`), and add an empty `## [Unreleased]` above
   it.
2. Update the links at the bottom of this file.
3. Set the same version in `project()` of `CMakeLists.txt`
   (eg: `VERSION 0.2.0`).
4. Set the new tag in the `GIT_TAG` of the FetchContent snippets, in
   `README.md` and `docs/mkdocs/getting-started.md` (eg: `GIT_TAG v0.2.0`).
5. Once it is merged, tag `main` and push the tag
   (`git tag v0.2.0 && git push origin v0.2.0`). The release workflow does the checks and creates the GitHub
   release.

## [Unreleased]

### Changed

- **Core**: the `Transform` of a child is now **local**, relative to its parent (like in Unity): moving, turning or scaling the parent moves its children. It was in world space before, so games that moved the children by hand must stop doing it.
- **Render**, **Physics**: the `RenderSystem` draws and the `CollisionSystem` compares each entity at its world transform (its `Transform` composed with its parents'). The `MovementSystem` still moves the local `Transform`.
- **Render**: the fields `start` and `end` of `Line` are renamed `from` and `to` (local points of its `Transform`). Games that build or read a `Line` must use the new names.
- **Render**: `Rectangle`, `Circle` and `Line` are now components: the `RenderSystem` draws them at the world transform of their entity, sorted by `layer` with the `Sprite`s and `Text`s. They can still be drawn by hand with `IRenderer::draw`.

### Added

- **Core**: `compose(parent, child)` in `Transform.hpp`, which moves a child `Transform` out of its parent (position, rotation, scale and origin).
- **Core**: `worldTransform(world, entity)` in `core/hierarchy/WorldTransform.hpp`, which gives the transform of an entity in the world. Parents without a `Transform` are skipped.
- **Scripting**: `getWorldTransform(entity)`, a copy of the transform in the world. `getBounds` uses the world transform.
- **Scene**: `Rectangle`, `Circle` and `Line` components in the scenes and prefabs. A negative `size`, `radius` or `outlineThickness` throws.
- **Scripting**: the `Rectangle`, `Circle` and `Line` types, with `getRectangle`, `hasRectangle`, `getCircle`, `hasCircle`, `getLine` and `hasLine`.
- **Tests**: unit tests of `compose`, of `worldTransform` (three levels, parent without `Transform`), of the collision of a child and of `getWorldTransform`.

## [0.5.1] - 2026-10-28

### Fixed

- **Scripting**: `kill` sends one `EntityDestroyed` per entity and frame, even when called several times, and none for an entity already dead.

## [0.5.0] - 2026-10-28

### Added

- **Scripting**: new module (`AMPERSAND_MODULE_SCRIPTING`) that runs Lua scripts with sol2 (Lua 5.4.7 and sol2 3.5.0 downloaded by CMake). The `Scripts` component lists the `.lua` files of an entity, and the `ScriptingSystem` gives each script its own `self` table and globals and calls its `start`, `update` and `onDestroy` functions.
- **Scripting**: errors of a script (missing file, syntax error, error in a function) turn it off without stopping the game, and are sent to the `EventQueue<ScriptError>` resource.
- **Scripting**: engine API for the scripts: the `Entity`, `Vec3` and `Transform` types, `getTransform` and `hasTransform`, `getParent` and `getChildren`, `destroy` (applied at the end of the frame through the `CommandQueue`), and the `input` table (`getAxis`, `getButton`, `getButtonDown`, `getButtonUp`) when the input module is turned on.
- **Scripting**: the `Vec2` and `Rect` (`intersects`) types, the `Velocity` with `getVelocity` and `hasVelocity`, the `Text` and `Color` with `getText` and `hasText`, the `BoxCollider` with `getBoxCollider`, `hasBoxCollider`, `getBounds` (box in the world) and `getColliders`, the `animation` table (`setBool`, `setFloat`, `playOnce`, `isPlayingAction`, `getState`) for the `AnimationStateMachine`, and the `AudioSource` with `getAudioSource` and `hasAudioSource` (`volume`, `loop`, `playing`), each when its module is turned on.
- **Scripting**: the game adds its own types and functions to the scripts with `ScriptingSystem::lua()`, and `ScriptingSystem::defer` runs an action at the end of the update, after every script.
- **Scene**: `Scripts` component (`"scripts"`, a list of paths relative to `gameRoot`), so scenes and prefabs give the scripts of their entities.
- **Tests**: unit tests of the `ScriptingSystem` (functions called, instances, globals, list changes, destroyed entities, errors), of its engine API (types, `Transform`, `destroy` and `input`), of `lua()` and `defer`, of velocities, texts, colliders, animation and audio sources, and of the `Scripts` of the scenes.
- **Scripting**: sol2 never prints the errors of the C++ functions called by a script (`SOL_PRINT_ERRORS=0`): they are only sent as a `ScriptError`.
- **Core**: engine events in `Events.hpp`: `EntityCreated` and `EntityDestroyed`, each sent only if the game added its `EventQueue`.
- **Scripting**: `kill(entity)`, which sends an `EntityDestroyed` then destroys the entity at the end of the frame like `destroy`.
- **Scene**: `instantiate` and `loadScene` send an `EntityCreated` for every entity they create, children included, once the prefab or the scene is fully loaded.
- **Tests**: unit tests of `kill` and its `EntityDestroyed`, and of the `EntityCreated` sent by `instantiate` and `loadScene` (children, persistent entities, failed scenes).

## [0.4.2] - 2026-10-26

### Changed

- **Scene**: `SceneManager::kVersion` is now `2`: every file must give `"version": 2`, a file with `"version": 1` is refused.
- **Scene**: the configuration gives `gameRoot` (the folder of the game) instead of `configRoot`. The paths of the scenes, prefabs and assets are relative to it.
- **Docs**: the game folder (`assets/`, `scripts/`, `world/` with `config.json`, `prefabs/` and `scenes/`) is documented in Scene, Getting started and the README.
- **Docs**: the FetchContent snippets use `GIT_TAG v0.4.1` (they still gave `v0.3.0`), and the release steps above say to update them.

## [0.4.1] - 2026-10-26

### Added

- **Core**: `has` takes several component types (`has<Transform, Velocity>(e)`): `true` if the entity has all of them.
- **Core**: `hasAny<A, B>(e)`: `true` if the entity has at least one of the types.

## [0.4.0] - 2026-10-26

### Added

- **Core**: parent and children of entities, stored in the `World`: `setParent`, `parent` and `children`, `destroy` destroys the children of the entity, and `setParent` throws `HierarchyCycle` instead of creating a cycle.
- **Scene**: new module (`AMPERSAND_MODULE_SCENE`) that builds the World from JSON files: a configuration (`configRoot`, `startScene`, `scenes` and the resources of the whole game), scenes (resources and entities of one level) and prefabs, read by the `SceneManager` with `loadConfig` and `loadScene`.
- **Scene**: prefabs merged field by field ([JSON Merge Patch](https://www.rfc-editor.org/rfc/rfc7386)), `instantiate`, persistent entities (`"persistent": true` and `dontDestroyOnLoad`) and assets (textures, fonts, sounds, music) loaded once per path.
- **Scene**: loaders for the types of the render, physics, input and audio modules, `registerResource` (with a `ResourceScope`) and `registerComponent` for the types of the game, and the `Backends` resource that keeps the `SfmlRenderer` and `SfmlAudioEngine` created from the files. The `Renderer`, `AudioListener` and `InputManager` are only allowed in the configuration.
- **Tests**: unit tests of the `SceneManager` (load, unload, prefabs, persistent entities and failure paths).

## [0.3.0] - 2026-10-23

Frame loop with the `Scheduler`, event and command queues, controllers and action map for the input, layers and animation state machine for the render.

### Added

- **Render**: `layer` field (default 0) on every drawable: `Sprite`, `Text`, `Rectangle`, `Circle` and `Line`.
- **Core**: `Scheduler`, which runs the systems in the order they were added and fills the `Clock`, and `FrameTimer`, which measures the real time of a frame, at most a given value.
- **Render**: `AnimationStateMachine` component, run by the `AnimationSystem`: named clips, ordered transitions on `params` (`"*"` for any state) and one-shot actions with `playOnce`.
- **Core**: `EventQueue<T>`, events sent by a system to the systems after it, and `Scheduler::addEventQueue<T>()`, which empties the queue at the end of every frame.
- **Core**: `CommandQueue`, a resource where systems record changes to the World (`destroy` for now), applied in order by the `Scheduler` at the end of the frame, so they are safe inside a loop over `owners<T>()`.
- **Render**: `framesFromGrid` helper and `FrameGrid` to build `Animation::frames` from a spritesheet grid of same-sized frames.
- **Input**: controllers: `JoystickButton` and `JoystickAxis`, `JoystickButtonPressed`, `JoystickButtonReleased` and `JoystickMoved` events, kept per controller and read with `getJoystickButton`, `getJoystickButtonDown`, `getJoystickButtonUp` and `getJoystickAxis`.
- **Input**: `get` functions on `InputManager` for keys and mouse buttons (`getKey`, `getKeyDown`, `getKeyUp`, `getMouseButton`...) and for any `Binding` (`getBinding`, `getBindingDown`, `getBindingUp`).
- **Input**: action map: named axes (`setAxis`, `getAxisRaw`) from a positive and a negative `Binding` and an optional controller axis with a dead zone and inversion, and named virtual buttons (`setButton`, `getButton`, `getButtonDown`, `getButtonUp`).
- **Render**: SFML conversion of the controller events to input events.
- **Tests**: unit tests of the `InputManager` (bindings, axes, virtual buttons, controllers) and of the controller events through the `InputSystem`.

### Changed

- **Render**: draw order is the `layer` of the drawable instead of the `z` of the `Transform`.
- **Input**: the event handling moved from the `InputSystem` to `InputManager::beginFrame` and `InputManager::apply`.
- **Input**: Changed the keys name to standard define in [unity's doc](https://docs.unity3d.com/Manual/class-InputManager.html).

## [0.2.0] - 2026-10-18

Binary serialization for the network module.

### Added

- **Net**: `BinaryWriter` and `BinaryReader` for big-endian serialization of integers, finite floats, enums and length-prefixed strings, with bounds and input validation.

### Changed

- README: fixed the organisation section (`include/` folder)
- Getting started: use the engine with FetchContent (`v0.1.0`).

## [0.1.0] - 2026-10-16

First release, with the very base of the engine.

### Added

- **Core**: sparse set ECS (`World`, `Entity`, components, resources, `ISystem`), `Transform` component, `Clock` resource, `Vec2`, `Vec3` and `Rect` math types.
- **Physics**: `Velocity`, `Gravity` and `BoxCollider` components, `GravityForce` and `Collisions` resources, `MovementSystem` and `CollisionSystem`.
- **Render**: `Sprite`, `Text` and `Animation` components, `Renderer`, `Camera` and `ClearColor` resources, `RenderSystem` and `AnimationSystem`, SFML backend behind `IRenderer`.
- **Input**: `InputManager` resource and `InputSystem`, keyboard keys by physical position, mouse buttons and position, focus loss.
- **Audio**: `AudioSource` component, `AudioListener` and `SceneMusic` resources, `AudioSystem`, SFML backend behind `IAudioEngine`.
- **Net**: non-blocking IPv4 `UdpSocket`, with Asio hidden inside the module.
- One CMake option and one static library per module.
- Documentation website.

[Unreleased]: https://github.com/Boz0o0/Ampersand-Engine/compare/v0.5.1...HEAD
[0.5.1]: https://github.com/Boz0o0/Ampersand-Engine/compare/v0.5.0...v0.5.1
[0.5.0]: https://github.com/Boz0o0/Ampersand-Engine/compare/v0.4.2...v0.5.0
[0.4.2]: https://github.com/Boz0o0/Ampersand-Engine/compare/v0.4.1...v0.4.2
[0.4.1]: https://github.com/Boz0o0/Ampersand-Engine/compare/v0.4.0...v0.4.1
[0.4.0]: https://github.com/Boz0o0/Ampersand-Engine/compare/v0.3.0...v0.4.0
[0.3.0]: https://github.com/Boz0o0/Ampersand-Engine/compare/v0.2.0...v0.3.0
[0.2.0]: https://github.com/Boz0o0/Ampersand-Engine/compare/v0.1.0...v0.2.0
[0.1.0]: https://github.com/Boz0o0/Ampersand-Engine/releases/tag/v0.1.0
