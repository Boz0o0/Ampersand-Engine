# Modules

The engine is split into modules, each built as its own static library. A game
only links the modules it uses, and a module that is turned off is not built
(nor its dependencies downloaded).
For why we split it this way, see [Design decisions](../decisions.md#separation-of-modules).

| Module  | CMake target        | Option                     | Needs           | Sources        | Tests            |
| ------- | ------------------- | -------------------------- | --------------- | -------------- | ---------------- |
| Core    | `ampersand_core`    | `AMPERSAND_MODULE_CORE`    |                 | `engine/src/core/`    | `engine/tests/core/`    |
| Physics | `ampersand_physics` | `AMPERSAND_MODULE_PHYSICS` | core            | `engine/src/physics/` | `engine/tests/physics/` |
| Render  | `ampersand_render`  | `AMPERSAND_MODULE_RENDER`  | core, input, SFML | `engine/src/render/`  | `engine/tests/render/`  |
| Input   | `ampersand_input`   | `AMPERSAND_MODULE_INPUT`   | core            | `engine/src/input/`   | `engine/tests/input/`   |
| Audio   | `ampersand_audio`   | `AMPERSAND_MODULE_AUDIO`   | core, SFML      | `engine/src/audio/`   | `engine/tests/audio/`   |
| Net     | `ampersand_net`     | `AMPERSAND_MODULE_NET`     | core, Asio      | `engine/src/net/`     | `engine/tests/net/`     |
| Scene   | `ampersand_scene`   | `AMPERSAND_MODULE_SCENE`   | core, nlohmann/json | `engine/src/scene/` | `engine/tests/scene/` |
| Scripting | `ampersand_scripting` | `AMPERSAND_MODULE_SCRIPTING` | core, Lua, sol2 | `engine/src/scripting/` | `engine/tests/scripting/` |

Every option is `ON` by default. Turn one off at configure time:

```sh
cmake -B build -DAMPERSAND_MODULE_RENDER=OFF
```

Linking a target also links the modules it needs: `ampersand_render` brings
`ampersand_input` and `ampersand_core`. `ampersand_scene` links every other
module that is turned on (except `net`), since it reads their types.

The headers of a module are in `engine/include/ampersand/<module>/`, with its
`components/`, `resources/` and `systems/` in folders of the same name.
