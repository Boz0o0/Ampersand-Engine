<p align="center">
  <img src="docs/assets/ampersand_logo.png" alt="Ampersand Engine logo" width="120">
</p>

<h1 align="center">Ampersand Engine</h1>

<p align="center">
  A modular C++ game engine built around a bitset-based ECS.
</p>

<p align="center">
  <a href="https://github.com/Boz0o0/Ampersand-Engine/actions/workflows/build-check.yml"><img src="https://github.com/Boz0o0/Ampersand-Engine/actions/workflows/build-check.yml/badge.svg" alt="Build Check"></a>
  <a href="https://github.com/Boz0o0/Ampersand-Engine/actions/workflows/unit-tests.yml"><img src="https://github.com/Boz0o0/Ampersand-Engine/actions/workflows/unit-tests.yml/badge.svg" alt="Unit Tests"></a>
  <a href="https://github.com/Boz0o0/Ampersand-Engine/actions/workflows/format-lint-check.yml"><img src="https://github.com/Boz0o0/Ampersand-Engine/actions/workflows/format-lint-check.yml/badge.svg" alt="Format & Lint Check"></a>
  <a href="https://boz0o0.github.io/Ampersand-Engine/"><img src="https://img.shields.io/badge/docs-online-blue" alt="Documentation"></a>
</p>

---

## Documentation

The full documentation can be found **[here](https://boz0o0.github.io/Ampersand-Engine/)**.

## Modules

The engine is split into libraries so that you only compile what you need.
Every module depends on `core`.

| Module    | CMake target         | Option                    |
| --------- | -------------------- | ------------------------- |
| Core      | `ampersand_core`     | `AMPERSAND_MODULE_CORE`    |
| Audio     | `ampersand_audio`    | `AMPERSAND_MODULE_AUDIO`   |
| Input     | `ampersand_input`    | `AMPERSAND_MODULE_INPUT`   |
| Network   | `ampersand_net`      | `AMPERSAND_MODULE_NET`     |
| Physics   | `ampersand_physics`  | `AMPERSAND_MODULE_PHYSICS` |
| Render    | `ampersand_render`   | `AMPERSAND_MODULE_RENDER`  |

All modules are enabled by default.

## Getting started

### Requirements

- CMake 3.16+
- A C++17 compiler (GCC, Clang or MSVC)
- Git (dependencies are fetched with [CPM.cmake](https://github.com/cpm-cmake/CPM.cmake))

### Build

```sh
cmake -B build -DCMAKE_BUILD_TYPE=Debug
cmake --build build --parallel
```

To build only some modules, turn the others off:

```sh
cmake -B build -DAMPERSAND_MODULE_NET=OFF -DAMPERSAND_MODULE_AUDIO=OFF
```

### Tests

Unit tests use [GoogleTest](https://github.com/google/googletest) and are built by default (`BUILD_TESTING=ON`).

```sh
ctest --test-dir build --output-on-failure
```

## Project structure

```
.
├── cmake/        # CMake helpers (CPM)
├── docs/         # MkDocs documentation
├── src/          # Engine modules (core, audio, input, net, physics, render)
└── tests/        # Unit tests, one folder per module
```
