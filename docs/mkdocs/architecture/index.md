# Architecture

This section describes how the engine is organised.
For the reasons behind these choices, see [Design decisions](../decisions.md).

## Overview

The engine is made of two layers:

- **The ECS** (Entity Component System) is the engine's core data model. Game objects are entities, their data lives in components, and the logic lives in systems. See [ECS](ecs.md).
- **The modules** split the engine's features into separate libraries. Every module is built on top of `core`. See [Modules](modules.md).
- **Physics** moves entities, makes them fall and finds the ones that touch. See [Physics](physics.md).
- **Rendering** provides the 2D drawing API, including sprites, primitives, text, views, transforms, and animation. See [2D rendering](rendering.md).
- **Audio** provides sound effects and music through an SFML-backed engine, plus the `AudioSource` and `AudioListener` components. See [Audio](audio.md).

## Module dependencies

```mermaid
graph BT
    audio[audio] --> core[core]
    input[input] --> core
    net[net] --> core
    physics[physics] --> core
    render[render] --> core
    audio --> render
    render --> input
```

`core` has no dependency on the other modules. The render module also depends on
input because its window event stream includes keyboard events. The audio module
depends on render, but render does not depend on audio.
