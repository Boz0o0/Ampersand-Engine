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
4. Once it is merged, tag `main` and push the tag
   (`git tag v0.2.0 && git push origin v0.2.0`). The release workflow does the checks and creates the GitHub
   release.

## [Unreleased]

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

[Unreleased]: https://github.com/Boz0o0/Ampersand-Engine/compare/v0.3.0...HEAD
[0.3.0]: https://github.com/Boz0o0/Ampersand-Engine/compare/v0.2.0...v0.3.0
[0.2.0]: https://github.com/Boz0o0/Ampersand-Engine/compare/v0.1.0...v0.2.0
[0.1.0]: https://github.com/Boz0o0/Ampersand-Engine/releases/tag/v0.1.0
