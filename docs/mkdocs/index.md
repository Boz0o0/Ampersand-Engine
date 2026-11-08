# Ampersand Engine

![Ampersand Engine logo](./assets/ampersand-logo-dark.svg#only-light){ width="200" }
![Ampersand Engine logo](./assets/ampersand-logo-light.svg#only-dark){ width="200" }

Ampersand Engine is a modular C++20 game engine built around a sparse set ECS.
It is split into independent modules, so a game only compiles what it uses.

## Where to go next

- **[Getting started](getting-started.md)**: requirements, building the engine, choosing modules and using it in a game.
- **[Architecture](architecture/index.md)**: how the engine is organised and how each module works.
- **[Design decisions](decisions.md)**: the choices we made, the alternatives we considered, and why.
- **[Contributing](contributing.md)**: how we work, our commits, code style, docs and tests.

## Modules

- **[ECS](architecture/ecs.md)** and **[Stats](architecture/stats.md)** (`core`): entities, components, systems and resources, and the in-game metrics.
- **[Physics](architecture/physics.md)**: movement, gravity and collisions.
- **[Input](architecture/input.md)**: keyboard, mouse and controllers of the frame, in one resource.
- **[Render](architecture/render.md)**: sprites, text, shapes, animation and camera, drawn by a `RenderSystem`.
- **[Audio](architecture/audio.md)**: sound effects and music, played by an `AudioSystem`.
- **[Net](architecture/net.md)**: a non-blocking UDP socket, and its traffic.
- **[Scene](architecture/scene.md)**: the World built from JSON configuration, scenes and prefabs.
- **[Scripting](architecture/scripting.md)**: the behaviour of the entities, in Lua.
- **[Editor](editor.md)**: a desktop app to edit the scenes without writing JSON.

## Used by

- **[R-Type](https://boz0o0.github.io/R-Type/)** ([source code](https://github.com/Boz0o0/R-Type)): a R-Type game built on top of Ampersand Engine.
